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
#include <fstream>
#include <thread>
#include <future>
#include <range/v3/all.hpp>
#include <numeric>

#ifdef _DEBUG
#pragma comment(lib, "opencv_world340d.lib")
#else
#pragma comment(lib, "opencv_world340.lib")
#endif

//#pragma comment(lib, "python36.lib")

#include <boost/serialization/split_free.hpp>  
#include <boost/serialization/vector.hpp>  
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

BOOST_SERIALIZATION_SPLIT_FREE(::cv::Mat)
namespace boost {
	namespace serialization {

		/** Serialization support for cv::Mat */
		template<class Archive>
		void save(Archive & ar, const ::cv::Mat& m, const unsigned int version)
		{
			size_t elem_size = m.elemSize();
			size_t elem_type = m.type();

			ar & m.cols;
			ar & m.rows;
			ar & elem_size;
			ar & elem_type;

			const size_t data_size = m.cols * m.rows * elem_size;
			ar & boost::serialization::make_array(m.ptr(), data_size);
		}

		/** Serialization support for cv::Mat */
		template <class Archive>
		void load(Archive & ar, ::cv::Mat& m, const unsigned int version)
		{
			int cols, rows;
			size_t elem_size, elem_type;

			ar & cols;
			ar & rows;
			ar & elem_size;
			ar & elem_type;

			m.create(rows, cols, elem_type);

			size_t data_size = m.cols * m.rows * elem_size;
			ar & boost::serialization::make_array(m.ptr(), data_size);
		}

		template<class Archive>
		void serial(Archive & ar, const std::vector<std::tuple<cv::Mat, cv::Mat>>& traningDatas, const unsigned int version)
		{
			long size = traningDatas.size();

			ar & size;

			for (const auto& traningData : traningDatas)
			{
				auto&[x, y] = traningData;
				boost::serialization::save(ar, x, version);
				boost::serialization::save(ar, y, version);
			}
		}

		template<class Archive>
		void serial(Archive & ar, const std::vector<std::tuple<cv::Mat, int>>& testDatas, const unsigned int version)
		{
			long size = testDatas.size();

			ar & size;

			for (const auto& testData : testDatas)
			{
				auto&[x, y] = testData;
				boost::serialization::save(ar, x, version);
				ar & y;
			}
		}

		template<class Archive>
		void deserial(Archive & ar, std::vector<std::tuple<cv::Mat, cv::Mat>>& traningDatas, const unsigned int version)
		{
			long size;

			ar & size;

			for (int i = 0; i < size; ++i)
			{
				cv::Mat x;
				cv::Mat y;

				boost::serialization::load(ar, x, version);
				boost::serialization::load(ar, y, version);

				traningDatas.push_back(std::make_tuple(x, y));
			}
		}

		template<class Archive>
		void deserial(Archive & ar, std::vector<std::tuple<cv::Mat, int>>& testDatas, const unsigned int version)
		{
			long size;

			ar & size;

			for (int i = 0; i < size; ++i)
			{
				cv::Mat x;
				int y;

				boost::serialization::load(ar, x, version);
				ar & y;

				testDatas.push_back(std::make_tuple(x, y));
			}
		}
	}
}

void printTest() {
	auto colors = std::vector<const char*>{ "red", "green", "blue", "yellow" };
	for (const auto&[i, color] : ranges::view::zip(ranges::view::iota(0), colors)) {
		std::cout << i << " " << color << std::endl;
	}
}

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

	for (const auto&[i, mat] : ranges::view::zip(ranges::view::iota(0), matArr))
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

				if ((x + 1) != mat.cols)
				{
					os << ", ";
				}
			}
			os << "]";

			if ((y + 1) != mat.rows)
			{
				os << ", " << std::endl;
				os << "    " << "    " << " ";
			}
		}

		os << "]";

		if ((i + 1) != matArr.size())
		{
			os << ", ";
		}

		os << std::endl;
	}

	//int i = 0; 
	//for (cv::Mat mat : matArr)
	//{
	//	os << "    " << i << ":" << std::endl;
	//	double* pMat = (double*)mat.data;

	//	os << "    " << "    " << "[";

	//	for (int y = 0; y < mat.rows; ++y)
	//	{
	//		os << "[";
	//		for (int x = 0; x < mat.cols; ++x)
	//		{
	//			os << *(pMat + x + y * mat.cols);

	//			/*if ((x + 1) == mat.cols)
	//			{
	//				os << ", ";
	//			}*/
	//		}
	//		os << "]";

	//		if ((y + 1) != mat.rows)
	//		{
	//			os << ", " << std::endl;
	//			os << "    " << "    " << " ";
	//		}
	//	}

	//	os << "]";

	//	if (++i != matArr.size())
	//	{
	//		os << ", ";
	//	}

	//	os << std::endl;
	//}

	os << "]" << std::endl;

	return os;
}

// 神经网络
class NetWork
{
public:
	NetWork(const std::initializer_list<int>&& sizes);
	void SGD(const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, int epochs, int mini_batch_size, float eta, const std::vector<std::tuple<cv::Mat, int>>& testData);

protected:
	cv::Mat feedforward(cv::Mat a);
	void updateMiniBatches(const std::vector<std::tuple<cv::Mat, cv::Mat>>& mini_batch, float eta);
	void updateMiniBatches(const std::vector<int>& orderNums, const int& startIndex, const int& mini_batch_size, const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, float eta);
	cv::Mat randn(int row, int col);
	cv::Mat sigmoid(const cv::Mat& z);
	cv::Mat sigmoid_prime(const cv::Mat& z);
	std::tuple<std::list<cv::Mat>, std::list<cv::Mat>> update(const cv::Mat& x, const cv::Mat& y);
	cv::Mat cost_derivative(cv::Mat output_activation, cv::Mat y);
	int evaluate(const std::vector<std::tuple<cv::Mat, int>>& testData);

public:
	// 每层的节点数量
	std::vector<int> mSizes;
	// 每层计算的偏差
	std::vector<cv::Mat> mBiases;
	// 每层计算的权重
	std::vector<cv::Mat> mWeight;
	// 层数
	int mNumLayers;
};

// T是文件原始数据 D是Mat数据类型
// 获取训练数据
template<typename T, typename D>
cv::Mat getData(std::string fileName)
{
	std::ifstream ifs;
	ifs.open(fileName, std::ios::binary);

	ifs.seekg(0, std::ios_base::end);

	// 文件字节大小
	long fileSize = ifs.tellg();

	// 训练数据数量
	int data_size = fileSize / sizeof(T);

	ifs.seekg(0, std::ios_base::beg);

	cv::Mat data;
	// 如果每个数据占用8个字节，则声明CV_64F类型
	if (sizeof(D) == 8)
	{
		data = cv::Mat{ data_size, 1, CV_64F, cv::Scalar(0.0) };
	}
	// 如果是其他字节数（4字节），则声明CV_32F类型
	else
	{
		data = cv::Mat{ data_size, 1, CV_32F, cv::Scalar(0.0f) };
	}

	int i = 0;
	T temp_data = 0.0;
	D* pBegin = (D*)data.data;

	while (i < data_size)
	{
		//ifs.read((char*)&(traning_data_x[i]), sizeof(float));
		ifs.read((char*)&temp_data, sizeof(T));

		//std::cout << i << " float = " << traning_data_x[i] << std::endl;
		pBegin[i] = temp_data;

		++i;
	}

	ifs.close();

	return data;
}

// T是文件原始数据
// 不存储在Mat中
// 返回训练数据指针和缓存的长度
template<typename T>
std::pair<T*,long> getDataRaw(std::string fileName)
{
	std::ifstream ifs;
	ifs.open(fileName, std::ios::binary);

	ifs.seekg(0, std::ios_base::end);

	long fileSize = ifs.tellg();

	int data_size = fileSize / sizeof(T);

	ifs.seekg(0, std::ios_base::beg);

	T* data = new T[data_size];

	int i = 0;

	while (i < data_size)
	{
		ifs.read((char*)&(data[i]), sizeof(T));
		++i;
	}
	ifs.close();

	return std::make_pair(data, data_size);
}

int main()
{
	NetWork net({ 784, 30, 10 });
	/*NetWork net({3, 2, 1});

	std::cout << "num_layers: " << net.mNumLayers << std::endl;
	std::cout << "mSizes: " << net.mSizes << std::endl;
	std::cout << "Biases: " << net.mBiases << std::endl;
	std::cout << "Weights: " << net.mWeight << std::endl;*/

	// float
	//cv::Mat traning_x = getData<float, float>("traning_data_x");
	//std::future<cv::Mat> future_traning_x = std::async(getData<float, float>, "traning_data_x");
	//std::ifstream ifs;
	//ifs.open("traning_data_x", std::ios::binary);

	//ifs.seekg(0, std::ios_base::end);

	//long fileSize = ifs.tellg();

	//int data_x_size = fileSize / 4;

	//ifs.seekg(0, std::ios_base::beg);

	////float* traning_data_x = new float[data_x_size];
	//cv::Mat traning_x{ data_x_size, 1, CV_32F, cv::Scalar(0.0f) };
	////std::cout << traning_x << std::endl;

	//{
	//	int i = 0;
	//	float temp_traning_data_y = 0.0f;
	//	float* pBegin = (float*)traning_x.data;

	//	while (i < data_x_size)
	//	{
	//		//ifs.read((char*)&(traning_data_x[i]), sizeof(float));
	//		ifs.read((char*)&traning_x, sizeof(float));

	//		//std::cout << i << " float = " << traning_data_x[i] << std::endl;
	//		pBegin[i] = temp_traning_data_y;

	//		++i;
	//	}
	//}

	//ifs.close();

	// double
	//cv::Mat tranning_y = getData<double, double>("traning_data_y");
	//std::future<cv::Mat> future_traning_y = std::async(getData<double, double>, "traning_data_y");
	//std::ifstream ifs_dy;
	//ifs_dy.open("traning_data_y", std::ios::binary);

	//ifs_dy.seekg(0, std::ios_base::end);

	////long fileSize = ifs_dy.tellg();
	//fileSize = ifs_dy.tellg();

	//int data_y_size = fileSize / 8;

	//ifs_dy.seekg(0, std::ios_base::beg);

	////double* traning_data_y = new double[data_y_size];
	//cv::Mat tranning_y{ data_y_size, 1, CV_64F, cv::Scalar(0.0) };

	//{
	//	int i = 0;
	//	double temp_traning_data_y = 0.0;
	//	double* pBegin = (double*)tranning_y.data;

	//	while (i < data_y_size)
	//	{
	//		//ifs_dy.read((char*)&(traning_data_y[i]), sizeof(double));
	//		ifs_dy.read((char*)&temp_traning_data_y, sizeof(double));

	//		//std::cout << i << " double = " << traning_data_y[i] << std::endl;
	//		pBegin[i] = temp_traning_data_y;

	//		++i;
	//	}
	//}
	//ifs_dy.close();

	// float
	//cv::Mat test_x = getData<float, float>("test_data_x");
	//std::future<cv::Mat> future_test_x = std::async(getData<float, float>, "test_data_x");
	//std::ifstream ifs_tx;
	//ifs_tx.open("test_data_x", std::ios::binary);

	//ifs_tx.seekg(0, std::ios_base::end);

	////long fileSize = ifs_tx.tellg();
	//fileSize = ifs_tx.tellg();

	//int test_x_size = fileSize / 4;

	//ifs_tx.seekg(0, std::ios_base::beg);

	////float* test_data_x = new float[test_x_size];
	//cv::Mat test_x{ test_x_size, 1, CV_32F, cv::Scalar(0.0f) };

	//{
	//	int i = 0;
	//	float temp_test_data_x = 0.0f;
	//	float* pBegin = (float*)test_x.data;

	//	while (i < test_x_size)
	//	{
	//		//ifs_tx.read((char*)&(test_data_x[i]), sizeof(float));
	//		ifs_tx.read((char*)&temp_test_data_x, sizeof(float));

	//		//std::cout << i << " float = " << test_data_x[i] << std::endl;
	//		pBegin[i] = temp_test_data_x;

	//		++i;
	//	}
	//}
	//ifs_tx.close();

	// long long
	//cv::Mat test_y = getData<long long, double>("test_data_y");
	//std::future<cv::Mat> future_test_y = std::async(getData<long long, double>, "test_data_y");
	//std::ifstream ifs_ty;
	//ifs_ty.open("test_data_y", std::ios::binary);

	//ifs_ty.seekg(0, std::ios_base::end);

	////long fileSize = ifs_ty.tellg();
	//fileSize = ifs_ty.tellg();

	//int test_y_size = fileSize / 8;

	//ifs_ty.seekg(0, std::ios_base::beg);

	////long long* test_data_y = new long long[test_y_size];
	//cv::Mat test_y{ test_y_size, 1, CV_64F, cv::Scalar(0.0) };

	//{
	//	int i = 0;
	//	long long temp_test_data_y = 0;

	//	double* pBegin = (double*)test_y.data;

	//	while (i < test_y_size)
	//	{
	//		//ifs_ty.read((char*)&(test_data_y[i]), sizeof(long long));
	//		ifs_ty.read((char*)&temp_test_data_y, sizeof(long long));

	//		//std::cout << i << " long long = " << test_data_y[i] << std::endl;
	//		pBegin[i] = temp_test_data_y;


	//		++i;
	//	}
	//}
	//ifs_ty.close();

	/*cv::Mat traning_y = future_traning_y.get();
	cv::Mat test_y = future_test_y.get();
	cv::Mat traning_x = future_traning_x.get();
	cv::Mat test_x = future_test_x.get();*/

	//// 2018 / 2
	/// 1 创建4个线程从最原始的裸数据里面读取数据
	//std::future<std::pair<long long*, long>> future_test_y = std::async(getDataRaw<long long>, "test_data_y");
	//std::future<std::pair<float*, long>> future_test_x = std::async(getDataRaw<float>, "test_data_x");
	//std::future<std::pair<double*, long>> future_traning_y = std::async(getDataRaw<double>, "traning_data_y");
	//std::future<std::pair<float*, long>> future_traning_x = std::async(getDataRaw<float>, "traning_data_x");

	//auto traning_y = future_traning_y.get();
	//auto test_y = future_test_y.get();
	//auto traning_x = future_traning_x.get();
	//auto test_x = future_test_x.get();
	//
	//// 打印数据验证是否正确
	///*for (int i = 0; i < traning_y.second; ++i)
	//{
	//	int j = i % 10;
	//	if (i != 0 && j == 0)
	//	{
	//		std::cout << ", " << std::endl;
	//	}

	//	std::cout << j << " : ";

	//	std::cout << traning_y.first[i] << std::endl;
	//}*/

	///*for (int i = 0; i < test_y.second; ++i)
	//{
	//	std::cout << test_y.first[i] << std::endl;
	//}*/

	//int count = traning_x.second / 784;

	//std::vector<std::tuple<cv::Mat, cv::Mat>> tranning_data;

	////for (int i = 0; i < count; ++i)
	//for (const auto& i : ranges::view::ints(0, count))
	//{
	//	// 训练输入
	//	cv::Mat mat_tx{ 784, 1, CV_64F, cv::Scalar(0.0f) };
	//	// 训练结果
	//	cv::Mat mat_ty{ 10, 1, CV_64F, cv::Scalar(0.0) };

	//	double* pBegin = (double*)mat_tx.data;
	//	float* pFirst = traning_x.first + i * 784;
	//	//for (int j = 0; j < 784; ++j)
	//	for (const auto& j : ranges::view::ints(0, 784))
	//	{
	//		pBegin[j] = pFirst[j];
	//	}

	//	memcpy(mat_ty.data, traning_y.first + i * 10, sizeof(double) * 10);

	//	tranning_data.push_back(std::make_tuple(mat_tx, mat_ty));
	//}

	//std::vector<std::tuple<cv::Mat, int>> test_data;

	//count = test_x.second / 784;

	////for (int i = 0; i < count; ++i)
	//for (const auto& i : ranges::view::ints(0, count))
	//{
	//	cv::Mat mat_tx{ 784, 1, CV_64F, cv::Scalar(0.0f) };

	//	double* pBegin = (double*)mat_tx.data;
	//	float* pFirst = test_x.first + i * 784;
	//	//for (int j = 0; j < 784; ++j)
	//	for (const auto& j : ranges::view::ints(0, 784))
	//	{
	//		pBegin[j] = pFirst[j];
	//	}

	//	test_data.push_back(std::make_tuple(mat_tx, test_y.first[i]));
	//}
	/// 1

	/// 2 将深度学习数据使用boost序列化方式存储在硬盘上
	//std::ofstream ofs("dldata.bin", std::ios::binary);
	//boost::archive::binary_oarchive oa(ofs);
	//boost::serialization::serial(oa, tranning_data, 1);
	//boost::serialization::serial(oa, test_data, 1);

	//ofs.flush();
	//ofs.close();
	/// 2
	
	/// 3 从Boost序列化数据里面读取深度学习训练数据
	std::vector<std::tuple<cv::Mat, cv::Mat>> tranning_data;
	std::vector<std::tuple<cv::Mat, int>> test_data;

	std::ifstream ifs("dldata.bin", std::ios::binary);
	boost::archive::binary_iarchive ia(ifs);
	boost::serialization::deserial(ia, tranning_data, 1);
	boost::serialization::deserial(ia, test_data, 1);

	// 打印验证
	/*for (auto& test : test_data)
	{
		auto& [x, y] = test;

		std::cout << "y = " << y << std::endl;
	}*/
	/// 3

	net.SGD(tranning_data, 30, 10, 0.5, test_data);

    return 0;
}

// 右值引用构造函数
NetWork::NetWork(const std::initializer_list<int>&& sizes):mSizes(sizes)
{
	mNumLayers = mSizes.size();

	//for (int i = 1; i < mSizes.size(); ++i)
	for (const auto& i : ranges::view::ints(1, mNumLayers))
	{
		mBiases.push_back(this->randn(mSizes[i], 1));
	}

	for (int i = 0, j = 1; i < mNumLayers && j < mNumLayers; ++i, ++j)
	{
		mWeight.push_back(this->randn(mSizes[j], mSizes[i]));
	}
}

// 开始训练
// 训练数据与测试数据中，矩阵的数据类型都是double CV_64F
void NetWork::SGD(const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, int epochs, int mini_batch_size, float eta, const std::vector<std::tuple<cv::Mat, int>>& testData)
{
	clock_t start = clock();
	// 未训练准确度
	if (testData.size() > 0)
	{
		int rightCount = this->evaluate(testData);

		std::cout << "Epoch -1 " << " : " << rightCount << " / " << testData.size() << std::endl;
	}
	clock_t ends = clock();
	std::cout << "测试的时间：" << (double)(ends - start) / CLOCKS_PER_SEC << "秒" << std::endl;
	
	//unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	//std::mt19937 gen(seed);

	std::vector<int> orderNum(training_data.size());

	/*for (int i = 0; i < orderNum.size(); ++i)
	{
		orderNum[i] = i;
	}*/

	std::iota(orderNum.begin(), orderNum.end(), 0);

	for (int i = 0; i < epochs; ++i)
	{
		start = clock();

		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::shuffle(orderNum.begin(), orderNum.end(), std::default_random_engine(seed));

		for (int i = 0, count = orderNum.size(); i < count; i += mini_batch_size)
		{
			/*std::vector<std::tuple<cv::Mat, cv::Mat>> miniBatches;

			for (int j = 0; j < mini_batch_size && (i + j) < count; ++j)
			{
				miniBatches.push_back(training_data[orderNum[i + j]]);
			}

			this->updateMiniBatches(miniBatches, eta);*/
			this->updateMiniBatches(orderNum, i, mini_batch_size, training_data, eta);
		}

		std::cout << "current i = " << i << std::endl;

		if (testData.size() > 0)
		{
			int rightCount = this->evaluate(testData);

			std::cout << "Epoch " << i << " : " << rightCount << " / " << testData.size() << std::endl;
		}

		std::cout << "Epoch " << i << " complete" << std::endl;

		ends = clock();
		std::cout << "训练测试花费的时间：" << (double)(ends - start) / CLOCKS_PER_SEC << "秒" << std::endl;
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
	static int count = 0;

	//std::cout << "current count = " << count++ << " eta = " << eta << " mini_batch size = " << mini_batch.size() << std::endl;

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
		/*auto x = std::get<0>(data);
		auto y = std::get<1>(data);*/
		auto[x, y] = data;

		//auto delta_nable_tuple = this->update(x, y);
		auto[delta_nable_b, delta_nable_w] = this->update(x, y);

		/*std::list<cv::Mat> delta_nable_b = std::get<0>(delta_nable_tuple);
		std::list<cv::Mat> delta_nable_w = std::get<1>(delta_nable_tuple);*/

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

void NetWork::updateMiniBatches(const std::vector<int>& orderNums, const int & startIndex, const int & mini_batch_size, const std::vector<std::tuple<cv::Mat, cv::Mat>>& training_data, float eta)
{
	static int count = 0;

	//std::cout << "current count = " << count++ << " eta = " << eta << " mini_batch size = " << mini_batch.size() << std::endl;

	std::list<cv::Mat> nabla_b;
	std::list<cv::Mat> nabla_w;

	for (const auto& b : this->mBiases)
	{
		nabla_b.push_back(cv::Mat::zeros(b.size(), b.type()));
	}

	for (const auto& w : this->mWeight)
	{
		nabla_w.push_back(cv::Mat::zeros(w.size(), w.type()));
	}

	//for (auto& data : mini_batch)
	for (int start = startIndex, i = 0, count = orderNums.size(); i < mini_batch_size && (startIndex + i) < count; ++i)
	{

		/*auto x = std::get<0>(data);
		auto y = std::get<1>(data);*/
		//auto[x, y] = data;
		const auto&[x, y] = training_data[orderNums[startIndex + i]];

		//auto delta_nable_tuple = this->update(x, y);
		auto&&[delta_nable_b, delta_nable_w] = this->update(x, y);

		/*std::list<cv::Mat> delta_nable_b = std::get<0>(delta_nable_tuple);
		std::list<cv::Mat> delta_nable_w = std::get<1>(delta_nable_tuple);*/

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

// 生成正太分布随机数
cv::Mat NetWork::randn(int row, int col)
{
	//default_random_engine generator;//如果用这个默认的引擎，每次生成的随机序列是相同的。
	std::random_device rd;
	std::mt19937 gen(rd());

	//normal(0,1)中0为均值，1为方差
	std::normal_distribution<double> normal(0, 1);

	cv::Mat mat{ row, col, CV_64F, cv::Scalar(0.0) };

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

cv::Mat NetWork::sigmoid(const cv::Mat& z)
{
	cv::Mat result;
	cv::exp(-1 * z, result);

	return 1.0 / (1.0 + result);
}

cv::Mat NetWork::sigmoid_prime(const cv::Mat& z)
{
//#if _DEBUG
	cv::Mat result_z = sigmoid(z);

	cv::Mat result_prime = 1 - result_z;

	return result_z.mul(result_prime);
//#else
//	return sigmoid(z) * (1 - sigmoid(z));
//#endif
}

std::tuple<std::list<cv::Mat>, std::list<cv::Mat>> NetWork::update(const cv::Mat& x, const cv::Mat& y)
{
	std::list<cv::Mat> nabla_b;
	std::list<cv::Mat> nabla_w;

	// 保存每层偏倒
	/*for (auto& b : this->mBiases) {
		nabla_b.push_back(cv::Mat::zeros(b.rows, b.cols, b.type()));
	}

	for (auto& w : this->mWeight) {
		nabla_w.push_back(cv::Mat::zeros(w.rows, w.cols, w.type()));
	}*/

	std::vector<cv::Mat> activations;
	activations.push_back(x);

	//cv::Mat activation = x;

	std::vector<cv::Mat> zs;


	int count = this->mBiases.size() > this->mWeight.size() ? this->mWeight.size() : this->mBiases.size();

	//for (int i = 0; i < count; ++i)
	for (const auto&[b, w] : ranges::view::zip(this->mBiases, this->mWeight))
	{
		try 
		{
			/*auto& b = this->mBiases[i];
			auto& w = this->mWeight[i];*/

			/*cv::Mat z = w * activation + b;
			zs.push_back(z);
			activation = this->sigmoid(z);
			activations.push_back(activation);*/

			zs.push_back(w * activations.back() + b);
			activations.push_back(this->sigmoid(zs.back()));
		}
		catch (cv::Exception e)
		{
			std::cout << e.code << "  " << e.msg << std::endl;
		}
	}

	//cv::Mat delta = this->cost_derivative(activations[activations.size() - 1], y).mul(this->sigmoid_prime(zs[zs.size() - 1]));
	cv::Mat delta = this->cost_derivative(activations.back(), y).mul(this->sigmoid_prime(zs.back()));
	cv::Mat transposeMat;
	cv::transpose(activations[activations.size() - 2], transposeMat);

	nabla_b.push_back(delta);
	nabla_w.push_back(delta * transposeMat);

	for (int i = 2; i < this->mNumLayers; ++i)
	{
		/*cv::Mat z = zs[zs.size() - i];

		cv::Mat sp = this->sigmoid_prime(z);*/
		cv::Mat sp = this->sigmoid_prime(zs[zs.size() - i]);

		cv::transpose(this->mWeight[this->mWeight.size() - i + 1], transposeMat);
		delta = (transposeMat * delta).mul(sp);

		nabla_b.push_front(delta);

		cv::transpose(activations[activations.size() - i - 1], transposeMat);
		nabla_w.push_front(delta * transposeMat);
	}

	return std::make_tuple(nabla_b, nabla_w);
}

cv::Mat NetWork::cost_derivative(cv::Mat output_activation, cv::Mat y)
{
	return output_activation - y;
}

/**
 * @brief 使用测试数据进行测试
 */
int NetWork::evaluate(const std::vector<std::tuple<cv::Mat, int>>& testData)
{
	std::vector<std::tuple<int, int>> testResult;

//#pragma omp parallel
	for (auto& data : testData)
	{
		/*cv::Mat x = std::get<0>(data);
		int y = std::get<1>(data);*/
		auto&[x, y] = data;

		cv::Mat result = this->feedforward(x);

		if (result.cols == 1)
		{
			int maxIndex = 0;
			double maxValue = 0.0;
			double* pBegin = (double*)result.data;

			for (int i = 0; i < result.rows; ++i)
			{
				//double temp = result.at<double>(i, 0);

				//if (temp > maxValue)
				if ((*(pBegin + i)) > maxValue)
				{
					maxIndex = i;
					//maxValue = temp;
					maxValue = *(pBegin + i);
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

//#pragma omp parallel
	for (const auto&[test, y] : testResult)
	{
		/*int test = std::get<0>(result);
		int y = std::get<0>(result);*/

		if (test == y)
		{
			++count;
		}
	}

	return count;
}
