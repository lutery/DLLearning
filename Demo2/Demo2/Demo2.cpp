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
	void GD(const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, int epochs);

protected:
	cv::Mat randn(int row, int col);
	cv::Mat sigmoid(cv::Mat z);
	void update(const cv::Mat& x, const cv::Mat& y);

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

void NetWork::GD(const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, int epochs)
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

		for (int order : orderNum)
		{
			auto& data = training_data[order];
			auto x = std::get<0>(data);
			auto y = std::get<1>(data);

			this->update(x, y);

			std::cout << "Epoch " << i << " complete" << std::endl;
		}
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
	cv::exp(z, result);

	return 1.0 / (1.0 + z);
}

void NetWork::update(const cv::Mat& x, const cv::Mat& y)
{
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
}
