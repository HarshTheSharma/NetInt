#include "NetInt.h"
#include <iostream>
// original code snippet from https://www.programiz.com/cpp-programming/examples/matrix-multiplication-function
using namespace std;

void enterData(NetInt firstMatrix[][10], NetInt secondMatrix[][10], NetInt rowFirst, NetInt columnFirst, NetInt rowSecond, NetInt columnSecond);
void multiplyMatrices(NetInt firstMatrix[][10], NetInt secondMatrix[][10], NetInt multResult[][10], NetInt rowFirst, NetInt columnFirst, NetInt rowSecond, NetInt columnSecond);
void display(NetInt mult[][10], NetInt rowFirst, NetInt columnSecond);

int main() {
	hideMessages(true);
	setWhitelist({"127.0.0.1", "10.0.0.1"});
	establishPort("8081");

	NetInt firstMatrix[10][10], secondMatrix[10][10], mult[10][10], rowFirst, columnFirst, rowSecond, columnSecond, i, j, k;

	cout << "Enter rows and column for first matrix: ";
	cin >> rowFirst >> columnFirst;

	cout << "Enter rows and column for second matrix: ";
	cin >> rowSecond >> columnSecond;

	// If colum of first matrix in not equal to row of second matrix, asking user to enter the size of matrix again.
	while (columnFirst != rowSecond) {
		cout << "                      Error! column of first matrix not equal to row of second." << endl;
		cout << "Enter rows and column for first matrix: ";
		cin >> rowFirst >> columnFirst;
		cout << "Enter rows and column for second matrix: ";
		cin >> rowSecond >> columnSecond;
	}

	// Function to take matrices data
	enterData(firstMatrix, secondMatrix, rowFirst, columnFirst, rowSecond, columnSecond);

	// Function to multiply two matrices.
	multiplyMatrices(firstMatrix, secondMatrix, mult, rowFirst, columnFirst, rowSecond, columnSecond);

	// Function to display resultant matrix after multiplication.
	display(mult, rowFirst, columnSecond);

	return 0;
}

void enterData(NetInt firstMatrix[][10], NetInt secondMatrix[][10], NetInt rowFirst, NetInt columnFirst, NetInt rowSecond, NetInt columnSecond) {
	NetInt i, j;
	cout << endl
		 << "Enter elements of matrix 1:" << endl;
	for (i = 0; i < rowFirst; ++i) {
		for (j = 0; j < columnFirst; ++j) {
			cout << "Enter elements a" << i + 1 << j + 1 << ": ";
			cin >> firstMatrix[i.value][j.value];
		}
	}

	cout << endl
		 << "Enter elements of matrix 2:" << endl;
	for (i = 0; i < rowSecond; ++i) {
		for (j = 0; j < columnSecond; ++j) {
			cout << "Enter elements b" << i + 1 << j + 1 << ": ";
			cin >> secondMatrix[i.value][j.value];
		}
	}
}

void multiplyMatrices(NetInt firstMatrix[][10], NetInt secondMatrix[][10], NetInt mult[][10], NetInt rowFirst, NetInt columnFirst, NetInt rowSecond, NetInt columnSecond) {
	NetInt i, j, k;

	// Initializing elements of matrix mult to 0.
	for (i = 0; i < rowFirst; ++i) {
		for (j = 0; j < columnSecond; ++j) {
			mult[i.value][j.value] = 0;
		}
	}

	// Multiplying matrix firstMatrix and secondMatrix and storing in array mult.
	for (i = 0; i < rowFirst; ++i) {
		for (j = 0; j < columnSecond; ++j) {
			for (k = 0; k < columnFirst; ++k) {
				mult[i.value][j.value] += firstMatrix[i.value][k.value] * secondMatrix[k.value][j.value];
			}
		}
	}
}

void display(NetInt mult[][10], NetInt rowFirst, NetInt columnSecond) {
	NetInt i, j;

	cout << "Output Matrix:" << endl;
	for (i = 0; i < rowFirst; ++i) {
		for (j = 0; j < columnSecond; ++j) {
			cout << mult[i.value][j.value] << " ";
			if (j == columnSecond - 1)
				cout << endl
					 << endl;
		}
	}
}