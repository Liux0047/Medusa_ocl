#ifndef COMMON_H_
#define COMMON_H_


#include <iostream>
#include <stdlib.h>
using namespace std;

void breakPoint() {
	int continue_key;
	cout << "Press any key to continue Medusa...";
	cin >> continue_key;
}


template <typename T>
int* generateQuota(int vertexCount, unsigned long totalCount) {
	int *randomInput = new int[vertexCount];
	unsigned long sum = 0;
	for (int i = 0; i < vertexCount; i++){
		randomInput[i] = rand() % 200 + 1;
		sum += randomInput[i];
	}
	double coefficient = (double)sum / totalCount;
	//normalize
	unsigned long intSum = 0;
	int reserve = 0;
	for (int i = 0; i < vertexCount; i++){
		randomInput[i] = static_cast<T> (randomInput[i] / coefficient);
		if (randomInput[i] == 0){
			randomInput[i] = 1;
			reserve++;
		}
		else if (randomInput[i] > reserve){
			randomInput[i] -= reserve;
			reserve = 0;
		}
		intSum += randomInput[i];
	}

	long remains = totalCount - intSum;
	if (remains <= 0){
		cout << "Invalid sample generated \n";
		breakPoint();
	}

	for (int i = 0; i < remains; i++) {
		int pos = rand() % vertexCount;
		randomInput[pos]++;
	}
		
	return randomInput;
}


template <typename T>
int* generateQuotaWithZero(int vertexCount, unsigned long totalCount) {
	int *randomInput = new int[vertexCount];
	unsigned long sum = 0;
	for (int i = 0; i < vertexCount; i++){
		randomInput[i] = rand() % 200 + 1;
		sum += randomInput[i];
	}
	double coefficient = (double)sum / totalCount;
	//normalize
	unsigned long intSum = 0;
	int reserve = 0;
	for (int i = 0; i < vertexCount; i++){
		randomInput[i] = static_cast<T> (randomInput[i] / coefficient);
		intSum += randomInput[i];
	}
	long remains = totalCount - intSum;
	if (remains <= 0 || randomInput[0] <= 0){
		cout << "Invalid sample generated \n";
		breakPoint();
	}

	for (int i = 0; i < remains; i++) {
		int pos = rand() % vertexCount;
		randomInput[pos]++;
	}

	return randomInput;
}

#endif