// Demo2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vector>
#include <random>
#include <iomanip>
#include <math.h>
#include <iostream>
#include <opencv2\opencv.hpp>
#include <list>
#include <tuple>
#include <time.h>
#include <chrono>

#ifdef _DEBUG
#pragma comment(lib, "opencv_world330d.lib")
#else
#pragma comment(lib, "opencv_world330.lib")
#endif

std::ostream& operator <<(std::ostream& os, const std::vector<int>& intArr)
{
	os << "[";

	int i = 0; 
	for (int integer : intArr)
	{
		os << integer;

		if (++i != intArr.size())
		{
			os << ", ";
		}
	}

	os << "]" << std::endl;

	return os;
}

std::ostream& operator <<(std::ostream& os, const std::vector<cv::Mat>& matArr)
{
	os << std::endl;

	os << "[" << std::endl;

	int i = 0; 
	for (cv::Mat mat : matArr)
	{
		os << "    " << i << ":" << std::endl;
		double* pMat = (double*)mat.data;

		os << "    " << "    " << "[";

		for (int y = 0; y < mat.rows; ++y)
		{
			os << "[";
			for (int x = 0; x < mat.cols; ++x)
			{
				os << *(pMat + x + y * mat.cols);

				/*if ((x + 1) == mat.cols)
				{
					os << ", ";
				}*/
			}
			os << "]";

			if ((y + 1) != mat.rows)
			{
				os << ", " << std::endl;
				os << "    " << "    " << " ";
			}
		}

		os << "]";

		if (++i != matArr.size())
		{
			os << ", ";
		}

		os << std::endl;
	}

	os << "]" << std::endl;

	return os;
}

class NetWork
{
public:
	NetWork(const std::initializer_list<int> sizes);
	void SGD(const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, int epochs, int mini_batch_size, float eta, const std::vector<std::tuple<cv::Mat, int>>& testData);

protected:
	cv::Mat feedforward(cv::Mat a);
	void updateMiniBatches(const std::vector<std::tuple<cv::Mat, cv::Mat>>& mini_batch, float eta);
	cv::Mat randn(int row, int col);
	cv::Mat sigmoid(cv::Mat z);
	cv::Mat sigmoid_prime(cv::Mat z);
	std::tuple<std::list<cv::Mat>, std::list<cv::Mat>> update(const cv::Mat& x, const cv::Mat& y);
	cv::Mat cost_derivative(cv::Mat output_activation, cv::Mat y);
	int evaluate(std::vector<std::tuple<cv::Mat, int>> testData);

public:
	std::vector<int> mSizes;
	std::vector<cv::Mat> mBiases;
	std::vector<cv::Mat> mWeight;
	int mNumLayers;
};

int main()
{
	NetWork net({ 3, 2, 1 });

	std::cout << "num_layers: " << net.mNumLayers << std::endl;
	std::cout << "mSizes: " << net.mSizes << std::endl;
	std::cout << "Biases: " << net.mBiases << std::endl;
	std::cout << "Weights: " << net.mWeight << std::endl;

    return 0;
}

NetWork::NetWork(const std::initializer_list<int> sizes)
{
	mSizes = sizes;
	mNumLayers = mSizes.size();

	for (int i = 1; i < mSizes.size(); ++i)
	{
		mBiases.push_back(this->randn(mSizes[i], 1));
	}

	for (int i = 0, j = 1; i < mSizes.size() && j < mSizes.size(); ++i, ++j)
	{
		mWeight.push_back(this->randn(mSizes[j], mSizes[i]));
	}
}

void NetWork::SGD(const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, int epochs, int mini_batch_size, float eta, const std::vector<std::tuple<cv::Mat, int>>& testData)
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	//std::mt19937 gen(seed);

	std::vector<int> orderNum(training_data.size());

	for (int i = 0; i < orderNum.size(); ++i)
	{
		orderNum[i] = i;
	}

	for (int i = 0; i < epochs; ++i)
	{
		std::shuffle(orderNum.begin(), orderNum.end(), std::default_random_engine(seed));

		for (int i = 0, count = orderNum.size(); i < count; i += mini_batch_size)
		{
			std::vector<std::tuple<cv::Mat, cv::Mat>> miniBatches;

			for (int j = 0; j < mini_batch_size && (i + j) < count; ++j)
			{
				miniBatches.push_back(training_data[orderNum[i + j]]);
			}

			this->updateMiniBatches(miniBatches, eta);
		}

		if (testData.size() > 0)
		{
			int rightCount = this->evaluate(testData);

			std::cout << "Epoch " << i << " : " << rightCount << " / " << testData.size() << std::endl;
		}

		std::cout << "Epoch " << i << " complete" << std::endl;
	}


}


cv::Mat NetWork::feedforward(cv::Mat a)
{
	auto bb = this->mBiases.begin();
	auto bw = this->mWeight.begin();

	for (; bb != this->mBiases.end() && bw != this->mWeight.end(); ++bb, ++bw)
	{
		a = this->sigmoid((*bw) * a + *bb);
	}

	return a;
}

void NetWork::updateMiniBatches(const std::vector<std::tuple<cv::Mat, cv::Mat>>& mini_batch, float eta)
{
	std::list<cv::Mat> nabla_b;
	std::list<cv::Mat> nabla_w;

	for (auto& b : this->mBiases)
	{
		nabla_b.push_back(cv::Mat::zeros(b.size(), b.type()));
	}

	for (auto& w : this->mWeight)
	{
		nabla_w.push_back(cv::Mat::zeros(w.size(), w.type()));
	}

	for (auto& data : mini_batch)
	{
		//auto& data = training_data[order];
		auto x = std::get<0>(data);
		auto y = std::get<1>(data);

		auto delta_nable_tuple = this->update(x, y);

		std::list<cv::Mat> delta_nable_b = std::get<0>(delta_nable_tuple);
		std::list<cv::Mat> delta_nable_w = std::get<1>(delta_nable_tuple);

		for (auto bnb = nabla_b.begin(), bdnb = delta_nable_b.begin(); bnb != nabla_b.end() && bdnb != delta_nable_b.end(); ++bnb, ++bdnb)
		{
			*bnb += *bdnb;
		}

		for (auto bnw = nabla_w.begin(), bdnw = delta_nable_w.begin(); bnw != nabla_w.end() && bdnw != delta_nable_w.end(); ++bnw, ++bdnw)
		{
			*bnw += *bdnw;
		}
	}

	auto bnb = nabla_b.begin();
	auto mb = this->mBiases.begin();
	for (; bnb != nabla_b.end() && mb != this->mBiases.end(); ++bnb, ++mb)
	{
		*mb = *mb - (eta) * (*bnb);
	}

	auto bnw = nabla_w.begin();
	auto mw = this->mWeight.begin();
	for (; bnw != nabla_w.end() && mw != this->mWeight.end(); ++bnw, ++mw)
	{
		*mw = *mw - (eta) * (*bnw);
	}
}

cv::Mat NetWork::randn(int row, int col)
{
	//default_random_engine generator;//如果用这个默认的引擎，每次生成的随机序列是相同的。
	std::random_device rd;
	std::mt19937 gen(rd());

	//normal(0,1)中0为均值，1为方差
	std::normal_distribution<double> normal(0, 1);

	cv::Mat mat(row, col, CV_64F);

	double* pMat = (double*)mat.data;

	for (int y = 0; y < row; ++y)
	{
		for (int x = 0; x < col; ++x)
		{
			*(pMat + x + y * col) = normal(gen);
		}
	}

	return mat;
}

cv::Mat NetWork::sigmoid(cv::Mat z)
{
	cv::Mat result;
	cv::exp(-1 * z, result);

	return 1.0 / (1.0 + result);
}

cv::Mat NetWork::sigmoid_prime(cv::Mat z)
{
	return sigmoid(z) * (1 - sigmoid(z));
}

std::tuple<std::list<cv::Mat>, std::list<cv::Mat>> NetWork::update(const cv::Mat& x, const cv::Mat& y)
{
	std::list<cv::Mat> nabla_b;
	std::list<cv::Mat> nabla_w;

	// 保存每层偏倒
	for (auto& b : this->mBiases) {
		nabla_b.push_back(cv::Mat::zeros(b.rows, b.cols, b.type()));
	}

	for (auto& w : this->mWeight) {
		nabla_w.push_back(cv::Mat::zeros(w.rows, w.cols, w.type()));
	}

	std::vector<cv::Mat> activations;
	activations.push_back(x);

	cv::Mat activation = x;

	std::vector<cv::Mat> zs;


	int count = this->mBiases.size() > this->mWeight.size() ? this->mWeight.size() : this->mBiases.size();

	for (int i = 0; i < count; ++i)
	{
		auto& b = this->mBiases[i];
		auto& w = this->mWeight[i];

		cv::Mat z = w * activation + b;

		zs.push_back(z);

		activation = this->sigmoid(z);

		activations.push_back(activation);
	}

	cv::Mat delta = this->cost_derivative(activations[activations.size() - 1], y).mul(this->sigmoid_prime(zs[zs.size() - 1]));
	cv::Mat transposeMat;
	cv::transpose(activations[activations.size() - 2], transposeMat);

	nabla_b.push_back(delta);
	nabla_w.push_back(delta * transposeMat);

	for (int i = zs.size() - 2; i >= 0; --i)
	{
		cv::Mat z = zs[i];

		cv::Mat sp = this->sigmoid_prime(z);

		cv::transpose(this->mWeight[i + 1], transposeMat);
		delta = (transposeMat * delta).mul(sp);

		nabla_b.push_front(delta);

		cv::transpose(activations[i - 1], transposeMat);
		nabla_w.push_front(delta * transposeMat);
	}

	return std::make_tuple(nabla_b, nabla_w);
}

cv::Mat NetWork::cost_derivative(cv::Mat output_activation, cv::Mat y)
{
	return output_activation - y;
}

int NetWork::evaluate(std::vector<std::tuple<cv::Mat, int>> testData)
{
	std::vector<std::tuple<int, int>> testResult;

	for (auto& data : testData)
	{
		cv::Mat x = std::get<0>(data);
		int y = std::get<1>(data);

		cv::Mat result = this->feedforward(x);

		if (result.cols == 1)
		{
			int maxIndex = 0;
			double maxValue = 0.0;

			for (int i = 0; i < result.rows; ++i)
			{
				double temp = result.at<double>(i, 0);

				if (temp > maxValue)
				{
					maxIndex = i;
					maxValue = temp;
				}
			}

			testResult.push_back(std::make_tuple(maxIndex, y));
		}
		else
		{
			std::cout << "计算就结果列数不正确" << std::endl;
		}
	}

	int count = 0;

	for (auto& result : testResult)
	{
		int test = std::get<0>(result);
		int y = std::get<0>(result);

		if (test == y)
		{
			++count;
		}
	}

	return count;
}
