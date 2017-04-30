#include "dynet/io.h"
#include "dynet/tensor.h"

namespace dynet {

void Packer::save(const ParameterCollection & model,
                const std::string & key, bool is_append) {
  std::string key_str(key);
  if (key.size() == 0) {
    key_str = model.get_namespace();
  }
  if (duplicate_key_check(key_str) == false) {
    DYNET_RUNTIME_ERR("You couldn't save ParameterCollections with the same key " + key_str + " in file: " + fn);
  }
  std::ofstream os;
  if (is_append) {
    os.open(fn_meta, std::ofstream::app);
  } else {
    os.open(fn_meta);
  }
  std::unordered_map<std::string, long long> offset_dict;
  os << key_str << ':' << offset;
  this->serialize(model, key, is_append, offset_dict);
  for (auto & kv : offset_dict) {
    os << '|' << kv.first << ':' << kv.second;
  }
  os << '\n';
  os.close();
}

void Packer::save(const ParameterCollection & model,
                const std::vector<std::string> & filter_lst,
                const std::string & key, bool is_append) {
  DYNET_RUNTIME_ERR("This interface is not implemented yet for Packer object.");
}

void Packer::save(const Parameter & param, const std::string & key, bool is_append) {
  std::string key_str(key);
  if (key.size() == 0) {
    key_str = param.get_fullname();
  }
  if (duplicate_key_check(key_str) == false) {
    DYNET_RUNTIME_ERR("You couldn't save Parameter with the same key " + key_str + " in file: " + fn);
  }
  std::ofstream os;
  if (is_append) {
    os.open(fn_meta, std::ofstream::app);
  } else {
    os.open(fn_meta);
  }
  os << key_str << ':' << offset << '\n';
  this->serialize(param, key, is_append);
  os.close();
}

void Packer::save(const LookupParameter & lookup_param, const std::string & key, bool is_append) {
  std::string key_str(key);
  if (key.size() == 0) {
    key_str = lookup_param.get_fullname();
  }
  if (duplicate_key_check(key_str) == false) {
    DYNET_RUNTIME_ERR("You couldn't save LookupParameter with the same key " + key_str + " in file: " + fn);
  }

  std::ofstream os;
  if (is_append) {
    os.open(fn_meta, std::ofstream::app);
  } else {
    os.open(fn_meta);
  }
  os << key_str << ':' << offset << '\n';
  this->serialize(lookup_param, key, is_append);
  os.close();
}

void Packer::populate(ParameterCollection & model, const std::string & key) {
  this->deserialize(model, key);
}

void Packer::populate(ParameterCollection & model,
                    const std::vector<std::string> & filter_lst,
                    const std::string & key) {
  DYNET_RUNTIME_ERR("This interface is not implemented yet for Packer object.");
}

void Packer::populate(Parameter & param,
                    const std::string & key) {
  this->deserialize(param, key);
}

void Packer::populate(Parameter & param,
                    const std::string & model_name,
                    const std::string & key) {
  this->deserialize(param, model_name, key);
}

void Packer::populate(LookupParameter & lookup_param,
                    const std::string & key) {
  this->deserialize(lookup_param, key);
}

void Packer::populate(LookupParameter & lookup_param,
                    const std::string & model_name,
                    const std::string & key) {
  this->deserialize(lookup_param, model_name, key);
}

Parameter Packer::load_param(ParameterCollection & model,
                           const std::string & key) {
  return this->deserialize_param(model, key);
}

Parameter Packer::load_param(ParameterCollection & model,
                           const std::string & model_name,
                           const std::string & key) {
  return this->deserialize_param(model, model_name, key);
}

LookupParameter Packer::load_lookup_param(ParameterCollection & model,
                                        const std::string & key) {
  return this->deserialize_lookup_param(model, key);
}

LookupParameter Packer::load_lookup_param(ParameterCollection & model,
                                        const std::string & model_name,
                                        const std::string & key) {
  return this->deserialize_lookup_param(model, model_name, key);
}

bool Packer::duplicate_key_check(const std::string & key) {
  std::ifstream f(fn_meta);
  std::string line;
  while (std::getline(f, line)) {
    auto kv = dynet::str_split(line, ':');
    if (kv[0] == key) return false;
  }
  f.close();
  return true;
}

void Packer::serialize(const ParameterCollection & model,
                     const std::string & key,
                     bool is_append,
                     std::unordered_map<std::string, long long> & offset_dict) {
  std::ofstream os;
  if (is_append) {
    os.open(fn, std::ofstream::app);
  } else {
    os.open(fn);
  }
  os.seekp(this->offset);
  os << '#' << std::endl; // beginning identifier
  auto all_params = model.get_parameter_storages_base();
  auto params = model.get_parameter_storages();
  auto lookup_params = model.get_lookup_parameter_storages();
  size_t i = 0, j = 0;
  for (size_t k = 0; k < all_params.size();  ++k) {
    if (i < params.size() && all_params[k] == params[i]) {
      os << "#Parameter#" << std::endl;
      offset_dict[params[i]->name] = os.tellp();
      serialize_parameter(os, params[i]);
      ++i;
    } else {
      os << "#LookupParameter#" << std::endl;
      offset_dict[lookup_params[j]->name] = os.tellp();
      serialize_lookup_parameter(os, lookup_params[j]);
      ++j;
    }
  }
  this->offset = os.tellp();
  os.close();
}

void Packer::serialize(const Parameter & param,
                     const std::string & key,
                     bool is_append) {
  std::ofstream os;
  if (is_append) {
    os.open(fn, std::ofstream::app);
  } else {
    os.open(fn);
  }
  os.seekp(this->offset);
  os << '#' << std::endl; // beginning identifier
  os << "#Parameter#" << std::endl;
  serialize_parameter(os, param.p);
  this->offset = os.tellp();
  os.close();
}

void Packer::serialize(const LookupParameter & lookup_param,
                     const std::string & key,
                     bool is_append) {
  std::ofstream os;
  if (is_append) {
    os.open(fn, std::ofstream::app);
  } else {
    os.open(fn);
  }
  os.seekp(this->offset);
  os << '#' << std::endl;
  os << "#LookupParameter#" << std::endl;
  serialize_lookup_parameter(os, lookup_param.p);
  this->offset = os.tellp();
  os.close();
}

void Packer::deserialize(ParameterCollection & model, const std::string & key) {
  std::ifstream meta_f(fn_meta);
  std::ifstream f(fn);
  // find the offset of the key
  long long local_offset = -1;

  std::string line;
  if (key.size() == 0) {
    // case for no key specified
    local_offset = 0;
  } else {
    while (std::getline(meta_f, line)) {
      auto tmp_str = dynet::str_split(line, '|').front();
      auto kv = dynet::str_split(tmp_str, ':');
      if (kv[0] == key) {
        local_offset = std::stoll(kv[1]);
        break;
      }
    }
  }
  if (local_offset == -1) {
    DYNET_RUNTIME_ERR("Load error: no such key: " + key);
  }

  f.seekg(local_offset);
  std::getline(f, line);
  if (line != "#") {
    DYNET_RUNTIME_ERR("Invalid model file format. Check this line: " + line);
  }

  std::string ns = model.get_namespace();
  // read parameters
  std::getline(f, line);
  while (line == "#Parameter#" || line == "#LookupParameter#") {
    if (line == "#Parameter#") {
      std::getline(f, line);
      if (dynet::startswith(line, ns) == false) {
        DYNET_RUNTIME_ERR("Inconsistent namespace error: " + line + " | " + ns);
      }
      auto name = line;

      Dim d;
      std::getline(f, line);
      std::istringstream iss(line); iss >> d;

      // add param into input model
      Parameter param = model.add_parameters(d);
      param.get_storage().name = name;

      // read param.get_storage().values
      std::getline(f, line);
      std::vector<real> params_l(d.size());
      std::istringstream iss2(line); iss2 >> params_l;
      TensorTools::SetElements(param.get_storage().values, params_l);

      // read param.get_storage().g
      std::getline(f, line);
      std::istringstream iss3(line); iss3 >> params_l;
      TensorTools::SetElements(param.get_storage().g, params_l);
      std::getline(f, line);
    } else {
      std::getline(f, line);
      if (dynet::startswith(line, ns) == false) {
        DYNET_RUNTIME_ERR("Inconsistent namespace error: " + line + " | " + ns);
      }
      auto name = line;

      Dim all_dim;
      std::getline(f, line);
      std::istringstream iss(line); iss >> all_dim;
      unsigned int N = all_dim.d[all_dim.nd - 1];

      Dim d;
      std::getline(f, line);
      std::istringstream iss2(line); iss2 >> d;

      // add lookup_param into input model
      LookupParameter lookup_param = model.add_lookup_parameters(N, d);
      lookup_param.get_storage().name = name;

      // read lookup_param.get_storage().all_values
      std::getline(f, line);
      std::vector<real> lookup_params_l(all_dim.size());
      std::istringstream iss3(line); iss3 >> lookup_params_l;
      TensorTools::SetElements(lookup_param.get_storage().all_values,
                               lookup_params_l);

      // read lookup_param.get_storage().all_grads
      std::getline(f, line);
      std::istringstream iss4(line); iss4 >> lookup_params_l;
      TensorTools::SetElements(lookup_param.get_storage().all_grads,
                               lookup_params_l);
      std::getline(f, line);
    }
  } // while

  if (line.size()) {
    if (line != "#") {
      DYNET_RUNTIME_ERR("Invalid model file format. Check this line: " + line);
    }
  }
  f.close();
  meta_f.close();
}
  
void Packer::deserialize(Parameter & param, const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(key));
  std::getline(f, line);
  if (line != "#") {
    DYNET_RUNTIME_ERR("Invalid model file format. Check this line: " + line);
  }
  std::getline(f, line); // #Parameter#
  std::getline(f, line); auto name = line;
  Dim d;
  std::getline(f, line);
  std::istringstream iss(line); iss >> d;
  if (param.get_storage().dim != d) {
    DYNET_RUNTIME_ERR("Dimension is not consistent.");
  }
  param.get_storage().name = name;
  
  std::getline(f, line);
  std::vector<real> params_l(d.size());
  std::istringstream iss2(line); iss2 >> params_l;
  TensorTools::SetElements(param.get_storage().values, params_l);

  std::getline(f, line);
  std::istringstream iss3(line); iss3 >> params_l;
  TensorTools::SetElements(param.get_storage().g, params_l);
  std::getline(f, line);
  f.close();
}

void Packer::deserialize(Parameter & param,
                       const std::string & model_name,
                       const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(model_name, key));
  std::getline(f, line);
  auto name = line;
  Dim d;
  std::getline(f, line);
  std::istringstream iss(line); iss >> d;
  if (param.get_storage().dim != d) {
    DYNET_RUNTIME_ERR("Dimension is not consistent.");
  }
  param.get_storage().name = name;

  std::getline(f, line);
  std::vector<real> params_l(d.size());
  std::istringstream iss2(line); iss2 >> params_l;
  TensorTools::SetElements(param.get_storage().values, params_l);

  std::getline(f, line);
  std::istringstream iss3(line); iss3 >> params_l;
  TensorTools::SetElements(param.get_storage().g, params_l);
  std::getline(f, line);
  f.close();
}

void Packer::deserialize(LookupParameter & lookup_param,
                       const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(key));
  std::getline(f, line);
  if (line != "#") {
    DYNET_RUNTIME_ERR("Invalid model file format. Check this line: " + line);
  }
  std::getline(f, line);
  std::getline(f, line);
  auto name = line;
  Dim all_dim;
  std::getline(f, line);
  std::istringstream iss(line); iss >> all_dim;
  
  std::getline(f, line);
  if (lookup_param.get_storage().all_dim != all_dim) {
    DYNET_RUNTIME_ERR("Dimension is not consistent.");
  }

  lookup_param.get_storage().name = name;
  
  std::getline(f, line);
  std::vector<real> lookup_params_l(all_dim.size());
  std::istringstream iss2(line); iss2 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_values,
                           lookup_params_l);
  std::getline(f, line);
  std::istringstream iss3(line); iss3 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_grads,
                           lookup_params_l);
  f.close();
}

void Packer::deserialize(LookupParameter & lookup_param,
                       const std::string & model_name,
                       const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(model_name, key));
  std::getline(f, line);
  auto name = line;
  Dim all_dim;
  std::getline(f, line);
  std::istringstream iss(line); iss >> all_dim;
  if (lookup_param.get_storage().all_dim != all_dim) {
    DYNET_RUNTIME_ERR("Dimension is not consistent.");
  }
  
  std::getline(f, line);
  lookup_param.get_storage().name = name;

  std::getline(f, line);
  std::vector<real> lookup_params_l(all_dim.size());
  std::istringstream iss2(line); iss2 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_values,
                           lookup_params_l);
  std::getline(f, line);
  std::istringstream iss3(line); iss3 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_grads,
                           lookup_params_l);
  f.close();
}

Parameter Packer::deserialize_param(ParameterCollection & model,
                                  const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(key));
  std::getline(f, line);
  if (line != "#") {
    DYNET_RUNTIME_ERR("Invalid model file format. Check this line: " + line);
  }
  std::getline(f, line); // #Parameter#
  std::getline(f, line); auto name = line;
  Dim d;
  std::getline(f, line);
  std::istringstream iss(line); iss >> d;
  Parameter param = model.add_parameters(d);
  param.get_storage().name = name;
  
  std::getline(f, line);
  std::vector<real> params_l(d.size());
  std::istringstream iss2(line); iss2 >> params_l;
  TensorTools::SetElements(param.get_storage().values, params_l);

  std::getline(f, line);
  std::istringstream iss3(line); iss3 >> params_l;
  TensorTools::SetElements(param.get_storage().g, params_l);
  std::getline(f, line);
  f.close();
  return param;
}

Parameter Packer::deserialize_param(ParameterCollection & model,
                            const std::string & model_name,
                            const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(model_name, key));
  std::getline(f, line);
  auto name = line;
  Dim d;
  std::getline(f, line);
  std::istringstream iss(line); iss >> d;
  
  Parameter param = model.add_parameters(d);
  param.get_storage().name = name;

  std::getline(f, line);
  std::vector<real> params_l(d.size());
  std::istringstream iss2(line); iss2 >> params_l;
  TensorTools::SetElements(param.get_storage().values, params_l);

  std::getline(f, line);
  std::istringstream iss3(line); iss3 >> params_l;
  TensorTools::SetElements(param.get_storage().g, params_l);
  std::getline(f, line);
  f.close();
  return param;
}

LookupParameter Packer::deserialize_lookup_param(ParameterCollection & model,
                                         const std::string & model_name,
                                         const std::string & key) {

  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(model_name, key));
  std::getline(f, line);
  auto name = line;
  Dim all_dim;
  std::getline(f, line);
  std::istringstream iss(line); iss >> all_dim;
  
  unsigned int N = all_dim.d[all_dim.nd - 1];
  Dim d;
  std::getline(f, line);
  std::istringstream iss2(line); iss2 >> d;
  
  LookupParameter lookup_param = model.add_lookup_parameters(N, d);
  lookup_param.get_storage().name = name;

  std::getline(f, line);
  std::vector<real> lookup_params_l(all_dim.size());
  std::istringstream iss3(line); iss3 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_values,
                           lookup_params_l);
  std::getline(f, line);
  std::istringstream iss4(line); iss4 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_grads,
                           lookup_params_l);
  f.close();
  return lookup_param;
}

LookupParameter Packer::deserialize_lookup_param(ParameterCollection & model,
                                               const std::string & key) {
  std::ifstream f(fn);
  std::string line;
  f.seekg(this->seek_offset(key));
  std::getline(f, line);
  if (line != "#") {
    DYNET_RUNTIME_ERR("Invalid model file format. Check this line: " + line);
  }
  std::getline(f, line);
  std::getline(f, line);
  auto name = line;
  Dim all_dim;
  std::getline(f, line);
  std::istringstream iss(line); iss >> all_dim;
  
  unsigned int N = all_dim.d[all_dim.nd - 1];
  Dim d;
  std::getline(f, line);
  std::istringstream iss2(line); iss2 >> d;
  LookupParameter lookup_param = model.add_lookup_parameters(N, d);

  if (lookup_param.get_storage().all_dim != all_dim) {
    DYNET_RUNTIME_ERR("Dimension is not consistent.");
  }

  lookup_param.get_storage().name = name;

  std::getline(f, line);
  std::vector<real> lookup_params_l(all_dim.size());
  std::istringstream iss3(line); iss3 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_values,
                           lookup_params_l);
  std::getline(f, line);
  std::istringstream iss4(line); iss4 >> lookup_params_l;
  TensorTools::SetElements(lookup_param.get_storage().all_grads,
                           lookup_params_l);
  f.close();
  return lookup_param;
}

void Packer::serialize_parameter(std::ofstream & os, const ParameterStorage *p) {
  os << p->name << '\n' << p->dim << '\n';
  os << dynet::as_vector(p->values);
  os << dynet::as_vector(p->g);
}

void Packer::serialize_lookup_parameter(std::ofstream & os,
                                      const LookupParameterStorage *p) {
  os << p->name << '\n' << p->all_dim << '\n' << p->dim << '\n';
  os << dynet::as_vector(p->all_values);
  os << dynet::as_vector(p->all_grads);
}

long long Packer::seek_offset(const std::string & key) {
  std::ifstream meta_f(fn_meta);
  std::string line;
  long long local_offset = -1;
  if (key.size() == 0) {
    local_offset = 0;
  } else {
    while (std::getline(meta_f, line)) {
      auto kv = dynet::str_split(line, ':');
      if (kv[0] == key) {
        local_offset = std::stoll(kv[1]);
        break;
      }
    }
  }
  if (local_offset== -1) {
    DYNET_RUNTIME_ERR("Load error: no such key: " + key);
  }
  meta_f.close();
  return local_offset;
}

long long Packer::seek_offset(const std::string & model_name,
                            const std::string & key) {
  std::ifstream meta_f(fn_meta);
  std::string line;
  long long local_offset = -1;
  bool model_exist = false;
  while (std::getline(meta_f, line)) {
    auto tmp_str = dynet::str_split(line, '|');
    auto kv = dynet::str_split(tmp_str.front(), ':');
    if (kv[0] == model_name) {
      model_exist = true;
      for (size_t k = 1; k < tmp_str.size(); ++k) {
        auto kkv = dynet::str_split(tmp_str[k], ':');
        if (kkv[0] == key) {
          local_offset = std::stoll(kkv[1]);
          break;
        }
      }
    }
  }
  if (model_exist == false) {
    DYNET_RUNTIME_ERR("Load error: no such key:" + key + " under model:" +  model_name);
  }
  meta_f.close();
  return local_offset;
}

} // namespace dynet