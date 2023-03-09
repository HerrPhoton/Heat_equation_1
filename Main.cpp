#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <chrono>

double **make_arr(size_t size, int lt = 10, int rt = 20, int lb = 20, int rb = 30)
{
	double **arr; //Сетка
	arr = new double *[size];

	for (size_t i = 0; i < size; i++)
	{
		arr[i] = new double[size];
		memset(arr[i], 0, size * sizeof(double));
	}

	//Установка значений в углах сетки
	arr[0][0] = lt;
	arr[0][size - 1] = rt;
	arr[size - 1][0] = lb;
	arr[size - 1][size - 1] = rb;

#pragma acc data copy(arr[0:size][0:size])
{
//Расчёт шага для каждой стороны сетки
double step1 = (arr[0][size - 1] - arr[0][0]) / (size - 1);
double step2 = (arr[size - 1][size - 1] - arr[0][size - 1]) / (size - 1);
double step3 =  (arr[size - 1][size - 1] - arr[size - 1][0]) / (size - 1);
double step4 = (arr[size - 1][0] - arr[0][0]) / (size - 1);

#pragma acc parallel loop independent
	for (size_t i = 1; i < size - 1; i++) //Заполнение сторон сетки с помощью линейной интерполяции между углами области
	{
		arr[0][i] = arr[0][0] + step1 * i;
		arr[i][size - 1] = arr[0][size - 1] + step2 * i;
		arr[size - 1][i] = arr[size - 1][0] + step3 * i;
		arr[i][0] = arr[0][0] + step4 * i;
	}
}
	return arr;
}

void delete_arr(double **arr, size_t size)
{
	for (size_t i = 0; i < size; i++)
		delete arr[i];

	delete arr;
}

void print_arr(double **arr, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		for (size_t j = 0; j < size; j++)
			std::cout << arr[i][j] << " ";

		std::cout << std::endl;
	}
}



int main(int argc, char **argv)
{
	auto begin = std::chrono::high_resolution_clock::now(); //Фиксация начального времени
 
	int size = 0; //Размер сетки
	int iters_cnt = 0; //Максимальнео количество итераций
	double error = 0; //Максимальное значение ошибки

	for (int i = 1; i < argc; i += 2) //Парсинг аргументов
	{
		std::string str = argv[i];

		if (str == "size")
			size = std::stoi(argv[i + 1]);

		else if (str == "error")
			error = std::stod(argv[i + 1]);

		else if (str == "iters")
			iters_cnt = std::stoi(argv[i + 1]);
	}
	
	double **arr = make_arr(size); //Создание сетки
	int passed_iter = 0; //Количество пройденных итераций
	double max_error = error + 1; //Максимальная достигнутая ошибка
	double prev = 0; //Предыдущее значение в ячейке сетки

#pragma acc data copy(arr[0:size][0:size])
	{
		while (max_error > error && passed_iter < iters_cnt)
		{
			max_error = 0;
			passed_iter++;

#pragma acc parallel loop collapse(2) reduction(max:max_error)	
			for (size_t i = 1; i < size - 1; i++)
				for (size_t j = 1; j < size - 1; j++)
				{
					prev = arr[i][j];
					arr[i][j] = 0.25 * (arr[i - 1][j] + arr[i + 1][j] + arr[i][j - 1] + arr[i][j + 1]); //Пятиточечный метод расчета сетки

					max_error = std::max(max_error, arr[i][j] - prev); //Фиксация максимальной ошибки на данной итерации
				}

		}
	}

	std::cout << "Passed iterations: " << passed_iter << "/" << iters_cnt << std::endl;
	std::cout << "Maximum error: " << max_error << "/" << error << std::endl;

	auto end = std::chrono::high_resolution_clock::now(); //Фиксация конечного времени
	double time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count(); //Общее время выполнения в микросекундах

	std::string unit = "mcs";
	std::string units[2] = {"ms", "s"}; //Массив доступных единиц измерения времени

	for (int i = 0; i < 2; i++) //Использование удобной единици измерения времени
	{
		if (time > 1000)
		{
			time /= 1000;
			unit = units[i];
		}

		else
			break;
	}

	std::cout << "Total execution time: " << time << " " << unit << std::endl;

//	print_arr(arr, size);
	delete_arr(arr, size); //Освобождение памяти, выделенной под сетку

	return 0;
}
