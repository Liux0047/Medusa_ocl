#ifndef COMMON_H_
#define COMMON_H_

template <typename T>
int* generateQuota(int vertexCount, int totalCount) {
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
	for (int i = 1; i < vertexCount; i++){
		randomInput[i] = static_cast<T> ((randomInput[i] + 0.5) / coefficient);
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
	randomInput[0] = totalCount - intSum;
	if (randomInput[0] <= 0){
		cout << "Invalid sample generated \n";
		breakPoint();
	}
	return randomInput;
}


#endif