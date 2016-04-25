#include "cnn/dict.h"
#include "cnn/expr.h"
#include "cnn/model.h"
#include "cnn/rnn.h"
#include "cnn/timing.h"
#include "cnn/training.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

#include "cnn/treelstm.h"
#include "conll_helper.cc"

using namespace cnn;

//TODO: add code for POS tag dictionary and dependency relation dictionary
cnn::Dict tokdict, sentitagdict, depreldict;
vector<unsigned> sentitaglist; // a list of all sentiment tags

const string UNK_STR = "UNK";

unsigned VOCAB_SIZE = 0, DEPREL_SIZE = 0, SENTI_TAG_SIZE = 0;

unsigned LAYERS = 1;
unsigned LSTM_INPUT_DIM = 300;
unsigned HIDDEN_DIM = 168;

template<class Builder>
struct TheirSentimentModel {
    LookupParameters* p_w;
    // TODO: input should also contain deprel to parent
    //LookupParameters* p_d;

    Parameters* p_tok2l;
//    Parameters* p_dep2l;
    Parameters* p_inp_bias;

    Parameters* p_root2senti;
    Parameters* p_sentibias;

    Builder treebuilder;

    explicit TheirSentimentModel(Model &model) :
            treebuilder(LAYERS, LSTM_INPUT_DIM, HIDDEN_DIM, &model) {
        p_w = model.add_lookup_parameters(VOCAB_SIZE, { LSTM_INPUT_DIM });
        //p_d = model.add_lookup_parameters(DEPREL_SIZE, { INPUT_DIM });

        p_tok2l = model.add_parameters( { HIDDEN_DIM, LSTM_INPUT_DIM });
        // p_dep2l = model.add_parameters( { HIDDEN_DIM, INPUT_DIM });
        p_inp_bias = model.add_parameters( { HIDDEN_DIM });
        // TODO: Change to add a regular BiLSTM below the tree

        p_root2senti = model.add_parameters( { SENTI_TAG_SIZE, HIDDEN_DIM });
        p_sentibias = model.add_parameters( { SENTI_TAG_SIZE });
    }

    Expression BuildTreeCompGraph(const DepTree& tree,
            const vector<int>& sentilabel, ComputationGraph* cg,
            int* prediction) {
        bool is_training = true;
        if (sentilabel.size() == 0) {
            is_training = false;
        }
        vector < Expression > errs; // the sum of this is to be returned...

        treebuilder.new_graph(*cg);
        treebuilder.start_new_sequence();
        treebuilder.initialize_structure(tree.numnodes);

        //Expression tok2l = parameter(*cg, p_tok2l);
        //  Expression dep2l = parameter(*cg, p_dep2l);
        //Expression inp_bias = parameter(*cg, p_inp_bias);

        Expression root2senti = parameter(*cg, p_root2senti);
        Expression senti_bias = parameter(*cg, p_sentibias);

        Expression h_root;
        for (unsigned node : tree.dfo) {
            Expression i_word = lookup(*cg, p_w, tree.sent[node]);
            //Expression i_deprel = lookup(*cg, p_d, tree.deprels[node]);
            // Expression input = affine_transform( { inp_bias, tok2l, i_word });
            //dep2l, i_deprel }); // TODO: add POS, dep rel and then use this

            vector<unsigned> clist; // TODO: why does it not compile with get_children?
            if (tree.children.find(node) != tree.children.end()) {
                clist = tree.children.find(node)->second;
            }
            h_root = treebuilder.add_input(node, clist, i_word);

            Expression i_root = affine_transform( { senti_bias, root2senti,
                    h_root });

            Expression prob_dist = log_softmax(i_root, sentitaglist);
            vector<float> prob_dist_vec = as_vector(cg->incremental_forward());

            int chosen_sentiment;
            if (is_training) {
                chosen_sentiment = sentilabel[node];
            } else { // the argmax

                double best_score = prob_dist_vec[0];
                chosen_sentiment = 0;
                for (unsigned i = 1; i < prob_dist_vec.size(); ++i) {
                    if (prob_dist_vec[i] > best_score) {
                        best_score = prob_dist_vec[i];
                        chosen_sentiment = i;
                    }
                }
                if (node == tree.root) {  // -- only for the root
                    *prediction = chosen_sentiment;
                }
            }

            Expression logprob = pick(prob_dist, chosen_sentiment);
            errs.push_back(logprob);
        }
        return -sum(errs);
    }
};