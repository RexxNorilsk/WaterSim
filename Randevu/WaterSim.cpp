#include <iostream>
#include "mpi.h"
#include <Windows.h>
#include <fstream>
#include <cstdlib>
#include <time.h>
#include <fstream>
#include <ctime>

//mpiexec -n 8 "C:\Parallel 3.2\WaterSim\x64\Debug\WaterSim.exe"

#define itsH 0
#define itsO 1
#define roomUnions 15
#define tagSurvey 0 
#define tagLive 1
#define wait 1
#define death 2
#define miss -1

using namespace std;


int randomRange(int max, int min = 1) {
	return min + rand() % (max - min);
}


void SurveyClients(int* clients, int count) {
	int buf;
	MPI_Status status;

	for (int i = 0; i < count; i++) {
		MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, tagSurvey, MPI_COMM_WORLD, &status);
		clients[status.MPI_SOURCE-1] = buf;
	}
}

int main(int argv, char** argc)
{
	
	int size, rank;
	if (MPI_Init(&argv, &argc) != MPI_SUCCESS)//Проверка на инициализацию
		return 1;
	if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS)//Получение размера коммуникатора
		return 2;
	if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)//Получение текущего ранга 
		return 3;
	if (size < 3)return 4;
	MPI_Status status;
	
		// Сервер
		if (rank == 0) {
			int* clients = new int[size - 1];

			//Приём клиентов и запись в массив
			SurveyClients(clients, size-1);
			int* buf;
			//Жизненный цикл комнаты
			for (int i = 0; i < roomUnions; i++) {
				cout << endl << "Server ready create new molecule" << endl;
				int* moleculeNow = new int[3];
				for (int j = 0; j < 3; j++)moleculeNow[j] = -1;
				
				for (int j = 0; j < 3; j++) {
					MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, tagLive, MPI_COMM_WORLD, &status);
					cout << "Molecule " << status.MPI_SOURCE << (clients[status.MPI_SOURCE - 1] == itsH ? " H " : " O ") << "is ready" << endl;

					int answer = miss;
					if (clients[status.MPI_SOURCE - 1] == itsH) {
						if (moleculeNow[0] < 0 || moleculeNow[1] < 0) {
							if (moleculeNow[0] < 0)
								moleculeNow[0] = status.MPI_SOURCE;
							else if (moleculeNow[1] < 0)
								moleculeNow[1] = status.MPI_SOURCE;
							answer = wait;
						}
					}
					else {
						if (moleculeNow[2] < 0) {
							moleculeNow[2] = status.MPI_SOURCE;
							answer = wait;
						}
					}
					//Ответ
					cout << "Molecule " << status.MPI_SOURCE << " answer is " << (answer == wait ? "wait" : "miss") << endl;
					MPI_Send(&answer, 1, MPI_INT, status.MPI_SOURCE, tagLive, MPI_COMM_WORLD);
					j = -1;
					for (int k = 0; k < 3; k++)if (moleculeNow[k] != -1)j++;
				}

				//Отправка начала объединения и времени жизни
				int time = randomRange(1000, 500);
				cout << "H20: ";
				for (int j = 0; j < 3; j++) {
					cout << moleculeNow[j] << " ";
					MPI_Send(&time, 1, MPI_INT, moleculeNow[j], tagLive, MPI_COMM_WORLD);
				}
				cout << " | Time: " << time << endl;
				
			}
			int fin = death;
			cout << endl << "====== Final stage ======" << endl;
			for (int j = 1; j < size; j++) {
				MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, tagLive, MPI_COMM_WORLD, &status);
				cout << "Check" << status.MPI_SOURCE;
				MPI_Send(&fin, 1, MPI_INT, status.MPI_SOURCE, tagLive, MPI_COMM_WORLD);
				cout << "...Ok" << endl;
			}
		}
		//Клиенты
		else {
			//Определение типа
			int type = rank%2==0 ? itsH : itsO;
			//Отправка данных о клиенте
			MPI_Send(&type, 1, MPI_INT, 0, tagSurvey, MPI_COMM_WORLD);

			int buf;
			while (true) {
				//Сон по времени
				Sleep(randomRange(500, 100));

				//Отправка о готовности
				MPI_Send(&type, 1, MPI_INT, 0, tagLive, MPI_COMM_WORLD);
				
				//Приём о дальнейших действиях
				MPI_Recv(&buf, 1, MPI_INT, 0, tagLive, MPI_COMM_WORLD, &status);
				
				bool isEnd = false;

				//Определение дальнейших действий
				switch (buf){
					case wait:
						//Ожидание
						MPI_Recv(&buf, 1, MPI_INT, 0, tagLive, MPI_COMM_WORLD, &status);
						Sleep(buf);
						break;
					case death:
						isEnd = true;
						break;
				}
				//cout << rank << " - " << (isEnd ? "END!" : "LIVE!") << endl;
				//Завершение цикла
				if (isEnd)break;
			}
		}
	MPI_Finalize();
}