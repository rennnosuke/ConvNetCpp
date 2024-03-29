//
// Created by kanairen on 2016/06/29.
//

#ifndef CONVNETCPP_CONVLAYER_H
#define CONVNETCPP_CONVLAYER_H

#include <random>
#import "GridLayer.h"

namespace ConvLayerConst {
    static const int T_WEIGHT_DISABLED = -1;
}

class ConvLayer2d_ : public GridLayer2d_ {

    /*
     * 入力を二次元画像とする、ニューラルネットワークの畳み込み層クラス
     */

private:

    // 成分数 (フィルタ幅)^2 × (入力チャネル) × （出力チャネル） のフィルタベクトル
    VectorXf h;

    // 入力画素(i, j)と重なるフィルタのインデックスを保持するベクトル
    MatrixXi t;

    void update(const MatrixXf &prev_output, const float learning_rate) {

        /*
         * フィルタ重み・バイアス値を更新する
         *
         * prev_output : 前層の出力
         * learning_rate : 学習率(0≦learning_rate≦1)
         */

#ifdef PROFILE_ENABLED
        time_t start = clock();
#endif

        float dw, db, d;

        const int n_o = n_out;
        const int n_i = n_in;
        const int n_d = n_data;
        int i_out, i_in, i_data;

        // W ← W - ε * dw / N　のうち、ε/Nを先に計算してしまう
        const float lr = learning_rate / n_d;

        for (i_in = 0; i_in < n_i; ++i_in) {
            for (i_out = 0; i_out < n_o; ++i_out) {
                if (t(i_out, i_in) != ConvLayerConst::T_WEIGHT_DISABLED) {
                    dw = 0.f;
                    db = 0.f;
                    for (i_data = 0; i_data < n_d; ++i_data) {
                        d = delta(i_out, i_data);
                        dw += d * prev_output(i_in, i_data);
                        db += d;
                    }
                    h[t(i_out, i_in)] -= lr * dw;
                }
            }
        }

        biases -= lr * delta.rowwise().sum();

        update_weights();

#ifdef PROFILE_ENABLED
        std::cout << "ConvLayer2d::update : " <<
        (float) (clock() - start) / CLOCKS_PER_SEC << "s" << std::endl;
#endif

    }

public:

    ConvLayer2d_(unsigned int n_data,
                 unsigned int input_width, unsigned int input_height,
                 unsigned int c_in, unsigned int c_out,
                 unsigned int kw, unsigned int kh,
                 unsigned int sx, unsigned int sy,
                 unsigned int px, unsigned int py,
                 float (*activation)(float), float (*grad_activation)(float),
                 bool is_filter_rand_init_enabled = true,
                 float filter_constant_value = 0.f,
                 bool is_dropout_enabled = false,
                 float dropout_rate = 0.5f)
            : GridLayer2d_(n_data, input_width, input_height, c_in, c_out, kw,
                           kh, sx, sy, px, py, activation, grad_activation,
                           false, is_dropout_enabled, dropout_rate),
              h(VectorXf::Constant(kw * kh * c_in * c_out,
                                   filter_constant_value)),
              t(MatrixXi::Constant(n_out, n_in,
                                   ConvLayerConst::T_WEIGHT_DISABLED)) {

        if (is_filter_rand_init_enabled) {

            // 乱数生成器
            std::random_device rnd;
            std::mt19937 mt(rnd());
            int f_in = c_in * kw * kh;
            int f_out = c_out * kw * kh;
            std::uniform_real_distribution<float> uniform(
                    -sqrtf(6.f / (f_in + f_out)),
                    sqrtf(6.f / (f_in + f_out)));

            // filter要素を初期化
            for (int i = 0; i < h.size(); ++i) {
                h(i) = uniform(mt);
            }

        }

        // tを初期化
        int i_in, j_out;
        for (int co = 0; co < c_out; ++co) {
            for (int y = 0; y <= input_height - kh; y += sy) {
                for (int x = 0; x <= input_width - kw; x += sx) {

                    j_out = co * output_height * output_width +
                            y / sy * output_width +
                            x / sx;

                    for (int ci = 0; ci < c_in; ++ci) {
                        for (int ky = 0; ky < kh; ++ky) {
                            for (int kx = 0; kx < kw; ++kx) {
                                i_in = ci * input_width * input_height +
                                       y * input_width + x +
                                       ky * input_width + kx;

                                t(j_out, i_in) = co * c_in * kh * kw +
                                                 ci * kh * kw +
                                                 ky * kw +
                                                 kx;

                            }
                        }
                    }

                }
            }
        }

        update_weights();

    }

    const VectorXf &get_filters() { return h; }

    void update_weights() {

        /*
         * 2D畳み込み版重み行列
         */

#ifdef PROFILE_ENABLED
        time_t start = clock();
#endif

        for (int j = 0; j < n_in; ++j) {
            for (int i = 0; i < n_out; ++i) {
                if (t(i, j) == ConvLayerConst::T_WEIGHT_DISABLED) {
                    weights(i, j) = 0.f;
                } else {
                    weights(i, j) = h(t(i, j));
                }
            }
        }

#ifdef PROFILE_ENABLED
        std::cout << "ConvLayer2d::update_weights : " <<
        (float) (clock() - start) / CLOCKS_PER_SEC << "s" << std::endl;
#endif

    }

};

#endif //CONVNETCPP_CONVLAYER_H
