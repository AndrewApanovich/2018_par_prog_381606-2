#include <iostream> 
#include <ctime>  
#include <mpi.h> 

#define ROOT 0
#define MAX_SHOW_SIZE 100

using namespace::std;

int  curr_rank_proc; // ������� ���� ��������
int  num_of_procs; // ����� ���������

double  time_seq_work_alg_radix = 0; 
double  time_pp_work_alg_radix = 0;

double* Create_and_init_arr(int size_arr)
{
	if (size_arr < 1)
		return NULL;

	double* arr;

	arr = new double[size_arr];
	srand((unsigned)time(NULL));

	for (int i = 0; i < size_arr; i++)
		arr[i] = rand() % 100 - 50 + (double)(rand()) / RAND_MAX + 0.00000001; // [-50, 51] - �������������� �����

	return arr;
}

// ���������� ������
void Show_arr(double* arr, int size_arr)
{
	if (arr == NULL || size_arr < 1)
		return;
	
	cout.precision(9);
		for (int i = 0; i < size_arr; i++)
			cout << arr[i] << " ";
		cout << endl;
	
}

// �������� � ��������
void Swap(int& arr_el_1, int& arr_el_2)
{
	int tmp = arr_el_1;
	arr_el_1 = arr_el_2;
	arr_el_2 = tmp;
}


// ��������� ������ ����� (������� � ������� �������� � ������ ���������) � ������ �������� (��� MPI_Scatterv � MPI_Gather)
void Calculate_work_and_displs(int* displs, int* send_num_work, int size_arr)
{
	int size_work = size_arr / num_of_procs,
		remainder = size_arr % num_of_procs;

	for (int i = 0; i < remainder; i++)
	{
		displs[i] = (size_work + 1) * i;
		send_num_work[i] = size_work + 1;
	}

	for (int i = remainder; i < num_of_procs; i++)
	{
		displs[i] = size_work * i + remainder; // � ������, ���� ������ �������������� �� ����������, �� �������� ��� ���� ��������� ����������� ���. ���� ����������, �� ������� �������� �����: displs[i] = size_work * i,
		//  � remainder == 0 � �� ����� �� �������� �� ���������� �������� � ���� ������
		send_num_work[i] = size_work;
	}
}

// ���������� ��������� ��� ���� double �� i-�� ����� ��� ������������� �����
void CountingSortPlus(double* arr_inp, double* arr_out, int size_arr, int byte_num)
{
	// ���� ���������:
	// ������ ���� ����� ����� 256 ��������� ��������� (�������� ����� �� 0 �� 255)
	// ����� ���� ����� � � � � �������� ��������� s_A � s_B ��������������
	// ���� A ������ ����� B, ���� s_A > s_B
	unsigned char* mas = (unsigned char*)arr_inp; // ��� double �������� ������������� � char - � 256 ��������� �����

	int counter[256];// �������� 256 ��������� ��������� ������ �����
	int offset;

	memset(counter, 0, sizeof(int) * 256); // ��������� ������ 256 ���� ������ (�.�. int �������� 4 �����, �� ����� 256 * 4 ���� ���������)

	// ����� � mas ���������� �������� ������������ (8 * i) ���� �� byte_num ���� (�������������� �������� ������ ��� ����)
	// 8 * i - �������� i-�� ������� ������� mas (�� ���� ��� ������� ����), � ����� � ������� ���������� byte_num �� ��������� ������� �����, �� �������� ������������� �� ������ ������
	// mas ���������� ������ � ����� �� 0 �� 255 (�� ���� ����� ��������� �����)
	for (int i = 0; i < size_arr; i++)
		counter[mas[8 * i + byte_num]]++; // counter ����������, ������� ����� ���� double �������� ������������ ������

	// ������ ���� ����� ��������� ����� byte_num, ������� ������������ � �����-���� ��������� double (���������, ������� �� ���� ����������, �� ������ ������ ��� �� ���������)
	int j = 0;
	for (j; j < 256; j++)
		if (counter[j] != 0)
			break;

	offset = counter[j];// ������ offset ����������, ������� ������� ��������� � ������������ ������ (����� ����������, ������� ����� ������� arr_out ����� ��� �����,
	// ���������� ���� � ������� ��������� j)
	counter[j] = 0;// ��� ������������� �������� ���������, ���������� ���� � ������� ��������� j. ������ ����� �������� ����� ����� "���������� ����" � ����� �������� � ������ ������� arr_out
	j++;

	// ����� ������� �������� � ���������� �� � counter
	for (j; j < 256; j++)
	{
		int tmp = counter[j];
		counter[j] = offset;
		offset += tmp;
	}

	for (int i = 0; i < size_arr; i++)
	{
		arr_out[counter[mas[8 * i + byte_num]]] = arr_inp[i];// counter �������� ��� ����������� ���������� �� ����������� ������������ ���������
		counter[mas[8 * i + byte_num]]++;// ����������� �������� �� 1 ��� ��������, ����������� ���� � ������� ��������� mas[8 * i + byte_num] (����� ��������� ��� �������� � ������ ������� arr_out)
	}


}

// ���������� ��������� ��� ���� double �� i-�� ����� ��� ������������� �����
void CountingSortMinus(double* arr_inp, double* arr_out, int size_arr, int byte_num)
{
	// ���� ���������:
	// ������ ���� ����� ����� 256 ��������� ��������� (�������� ����� �� 0 �� 255)
	// ����� ���� ����� � � � � �������� ��������� s_A � s_B ��������������
	// ���� A ������ ����� B, ���� s_A < s_B
	unsigned char* mas = (unsigned char*)arr_inp; // ��� double �������� ������������� � char - � 256 ��������� �����

	int counter[256];// �������� 256 ��������� ��������� ������ �����
	int offset;

	memset(counter, 0, sizeof(int) * 256); // ��������� ������ 256 ���� ������ (�.�. int �������� 4 �����, �� ����� 256 * 4 ���� ���������)

	// ����� � mas ���������� �������� ������������ (8 * i) ���� �� byte_num ���� (�������������� �������� ������ ��� ����)
	// 8 * i - �������� i-�� ������� ������� mas (�� ���� ��� ������� ����), � ����� � ������� ���������� byte_num �� ��������� ������� �����, �� �������� ������������� �� ������ ������
	// mas ���������� ������ � ����� �� 0 �� 255 (�� ���� ����� ��������� �����)
	for (int i = 0; i < size_arr; i++)
		counter[mas[8 * i + byte_num]]++; // counter ����������, ������� ����� ���� double �������� ������������ ������

	// ������ ���� ����� ��������� ����� byte_num, ������� ������������ � �����-���� ��������� double (���������, ������� �� ���� ����������, �� ������ ������ ��� �� ���������)
	int j = 255;
	for (j; j >= 0; j--)
		if (counter[j] != 0)
			break;

	offset = counter[j];// ������ offset ����������, ������� ������� ��������� � ������������ ������ (����� ����������, ������� ����� ������� arr_out ����� ��� �����,
	// ���������� ���� � ������� ��������� j)
	counter[j] = 0;// ��� ������������� �������� ���������, ���������� ���� � ������� ��������� j. ������ ����� �������� ����� ����� "���������� ����" � ����� �������� � ������ ������� arr_out
	j--;

	// ����� ������� �������� � ���������� �� � counter
	for (j; j >= 0; j--)
	{
		int tmp = counter[j];
		counter[j] = offset;
		offset += tmp;
	}

	for (int i = 0; i < size_arr; i++)
	{
		arr_out[counter[mas[8 * i + byte_num]]] = arr_inp[i];// counter �������� ��� ����������� ���������� �� ����������� ������������ ���������
		counter[mas[8 * i + byte_num]]++;// ����������� �������� �� 1 ��� ��������, ����������� ���� � ������� ��������� mas[8 * i + byte_num] (����� ��������� ��� �������� � ������ ������� arr_out)
	}

}

void LSDSortDouble(double* arr_inp, int size_arr)
{
	double* arr_inp_plus;
	double* arr_inp_minus;
	double* arr_out_plus;
	double* arr_out_minus;
	int size_arr_plus = 0,
		size_arr_minus = 0;

	int counter_arr_plus = 0,
		counter_arr_minus = 0;

	for (int i = 0; i < size_arr; i++)// ���������� ����� ������������� � ������������� ���������
		if (arr_inp[i] > 0)
			size_arr_plus++;
		else
			size_arr_minus++;

	arr_inp_plus = new double[size_arr_plus];
	arr_inp_minus = new double[size_arr_minus];
	arr_out_plus = new double[size_arr_plus];
	arr_out_minus = new double[size_arr_minus];

	// ��������� + � - �������� � �������������� �������
	for (int i = 0; i < size_arr; i++)
		if (arr_inp[i] > 0)
			arr_inp_plus[counter_arr_plus++] = arr_inp[i];
		else
			arr_inp_minus[counter_arr_minus++] = arr_inp[i];

	// ��������� ������������� ������
	if (size_arr_plus > 0)
	{
		CountingSortPlus(arr_inp_plus, arr_out_plus, size_arr_plus, 0);
		CountingSortPlus(arr_out_plus, arr_inp_plus, size_arr_plus, 1);
		CountingSortPlus(arr_inp_plus, arr_out_plus, size_arr_plus, 2);
		CountingSortPlus(arr_out_plus, arr_inp_plus, size_arr_plus, 3);
		CountingSortPlus(arr_inp_plus, arr_out_plus, size_arr_plus, 4);
		CountingSortPlus(arr_out_plus, arr_inp_plus, size_arr_plus, 5);
		CountingSortPlus(arr_inp_plus, arr_out_plus, size_arr_plus, 6);
		CountingSortPlus(arr_out_plus, arr_inp_plus, size_arr_plus, 7);
	}

	// ��������� ������������� ������
	if (size_arr_minus > 0)
	{
		CountingSortMinus(arr_inp_minus, arr_out_minus, size_arr_minus, 0);
		CountingSortMinus(arr_out_minus, arr_inp_minus, size_arr_minus, 1);
		CountingSortMinus(arr_inp_minus, arr_out_minus, size_arr_minus, 2);
		CountingSortMinus(arr_out_minus, arr_inp_minus, size_arr_minus, 3);
		CountingSortMinus(arr_inp_minus, arr_out_minus, size_arr_minus, 4);
		CountingSortMinus(arr_out_minus, arr_inp_minus, size_arr_minus, 5);
		CountingSortMinus(arr_inp_minus, arr_out_minus, size_arr_minus, 6);
		CountingSortMinus(arr_out_minus, arr_inp_minus, size_arr_minus, 7);
	}

	// �������
	for (int i = 0; i < size_arr_minus; i++)
		arr_inp[i] = arr_inp_minus[i];

	for (int i = 0; i < size_arr_plus; i++)
		arr_inp[i + size_arr_minus] = arr_inp_plus[i];

}

void Compare_split_right(double* buffer_curr_proc, int size_curr_proc_buffer, int id_proc_right, int size_buffer_proc_right)
{
	double* buffer_proc_recv = new double[size_buffer_proc_right];
	double* merge_arr = new double[size_curr_proc_buffer + size_buffer_proc_right];
	MPI_Status status;

	/* INTERCHANGE */

	MPI_Sendrecv(buffer_curr_proc, size_curr_proc_buffer, MPI_DOUBLE, id_proc_right, 1, buffer_proc_recv, size_buffer_proc_right, MPI_DOUBLE, id_proc_right, 1, MPI_COMM_WORLD, &status);

	/* MERGE */

	// ��� �������, ��� ������ �����������, ���������� �������� ������� �������

	int index_buffer_curr_proc = 0,
		index_buffer_right_proc = 0,
		index_buffer_merge = 0;

	// ���� �������, ������ merge_arr �� ���� ����� ������ ����������
	while (index_buffer_curr_proc < size_curr_proc_buffer && index_buffer_right_proc < size_buffer_proc_right)
		if (buffer_curr_proc[index_buffer_curr_proc] < buffer_proc_recv[index_buffer_right_proc])
			merge_arr[index_buffer_merge++] = buffer_curr_proc[index_buffer_curr_proc++];
		else
			merge_arr[index_buffer_merge++] = buffer_proc_recv[index_buffer_right_proc++];

	// ���� ����� �� while ��������� ��-�� ������� index_buffer_curr_proc < size_curr_proc_buffer, �� ��������� ���������� �������� �� buffer_proc_recv
	while (index_buffer_right_proc < size_buffer_proc_right) // � ����� ������, ����� �������� ����������, �.�. ������ ������� �������� ���� ����� �� ������, ��� � ������� �������, ���� �� 1 ������
		merge_arr[index_buffer_merge++] = buffer_proc_recv[index_buffer_right_proc++];
	// ���� ����� �� while ��������� ��-�� ������� index_buffer_right_proc < size_buffer_proc_right, �� ��������� ���������� �������� �� buffer_curr_proc
	while (index_buffer_curr_proc < size_curr_proc_buffer)
		merge_arr[index_buffer_merge++] = buffer_curr_proc[index_buffer_curr_proc++];

	/* Split */

	// ��������� merge_arr �� 2 �����: ����� ����� �������� size_curr_proc_buffer ����������� �������� � ������ id_proc_right - 1 (������� ��������),
	// � ������ ����� �������� size_buffer_proc_right ����������� �������� � ������ id_proc_right (������� ��������) (�� ��� ����� ��������� � �-��� Compare_split_left())

	for (int i = 0; i < size_curr_proc_buffer; i++)
		buffer_curr_proc[i] = merge_arr[i];

}


void Compare_split_left(double* buffer_curr_proc, int size_curr_proc_buffer, int id_proc_left, int size_buffer_proc_left)
{
	double* buffer_proc_recv = new double[size_buffer_proc_left];
	double* merge_arr = new double[size_curr_proc_buffer + size_buffer_proc_left];
	MPI_Status status;

	/* INTERCHANGE */

	MPI_Sendrecv(buffer_curr_proc, size_curr_proc_buffer, MPI_DOUBLE, id_proc_left, 1, buffer_proc_recv, size_buffer_proc_left, MPI_DOUBLE, id_proc_left, 1, MPI_COMM_WORLD, &status);

	/* MERGE */

	// ��� �������, ��� ������ �����������, ���������� �������� ������� �������

	int index_buffer_curr_proc = 0,
		index_buffer_left_proc = 0,
		index_buffer_merge = 0;

	// ���� �������, ������ merge_arr �� ���� ����� ������ ����������
	while (index_buffer_curr_proc < size_curr_proc_buffer && index_buffer_left_proc < size_buffer_proc_left)
		if (buffer_curr_proc[index_buffer_curr_proc] < buffer_proc_recv[index_buffer_left_proc])
			merge_arr[index_buffer_merge++] = buffer_curr_proc[index_buffer_curr_proc++];
		else
			merge_arr[index_buffer_merge++] = buffer_proc_recv[index_buffer_left_proc++];

	// ���� ����� �� while ��������� ��-�� ������� index_buffer_curr_proc < size_curr_proc_buffer, �� ��������� ���������� �������� �� buffer_proc_recv
	while (index_buffer_left_proc < size_buffer_proc_left)
		merge_arr[index_buffer_merge++] = buffer_proc_recv[index_buffer_left_proc++];
	// ���� ����� �� while ��������� ��-�� ������� index_buffer_left_proc < size_buffer_proc_left, �� ��������� ���������� �������� �� buffer_curr_proc
	while (index_buffer_curr_proc < size_curr_proc_buffer) // � ����� ������, ����� �������� ����������, �.�. ����� ������� �������� ���� ����� �� ������, ��� � ������� �������, ���� �� 1 ������
		merge_arr[index_buffer_merge++] = buffer_curr_proc[index_buffer_curr_proc++];

	/* Split */

	// ��������� merge_arr �� 2 �����: ����� ����� �������� size_curr_proc_buffer ����������� �������� � ������ id_proc_right - 1 (������� ��������),
	// � ������ ����� �������� size_buffer_proc_right ����������� �������� � ������ id_proc_right (������� ��������) (�� ��� ����� ��������� � �-��� Compare_split_left())

	for (int i = 0; i < size_curr_proc_buffer; i++)
		buffer_curr_proc[i] = merge_arr[size_buffer_proc_left + i];
}

// ���������� ������������
void Sort_pp(double* recv_buffer, int* send_num_work)
{
	for (int i = 0; i < num_of_procs; i++)
		if (i % 2 == 1) // ���� �����������, �� ��� �������� ��������
			if (curr_rank_proc % 2 == 1)
			{
				if (curr_rank_proc < num_of_procs - 1) // ��������� - ����� � ������� ������
					Compare_split_right(recv_buffer, send_num_work[curr_rank_proc], curr_rank_proc + 1, send_num_work[curr_rank_proc + 1]);
			}
			else
			{
				if (curr_rank_proc > 0)// ����� ��������� - ����� � ������� �����
					Compare_split_left(recv_buffer, send_num_work[curr_rank_proc], curr_rank_proc - 1, send_num_work[curr_rank_proc - 1]);
			}
		else // ����� ��� ������ ��������
			if (curr_rank_proc % 2 == 0)
			{
				if (curr_rank_proc < num_of_procs - 1) // ��������� - ����� � ������� ������
					Compare_split_right(recv_buffer, send_num_work[curr_rank_proc], curr_rank_proc + 1, send_num_work[curr_rank_proc + 1]);
			}
			else // ����� ��������� - ����� � ������� �����
				Compare_split_left(recv_buffer, send_num_work[curr_rank_proc], curr_rank_proc - 1, send_num_work[curr_rank_proc - 1]);

}


int main(int argc, char* argv[])
{

	int size_arr = 0;
	int* displs; // ������ �������� ������������ ������ ������ test_arr
	int* send_num_work; // ������ ���������� ����� ��� ������� ��������
	double* recv_buffer;
	double* test_arr_seq_radix = NULL;
	double* test_arr_pp_radix= NULL;
	

	double seq_alg_time_start_radix = 0;
	double seq_alg_time_end_radix = 0;
	double pp_alg_time_start_radix = 0;
	double pp_alg_time_end_radix = 0;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &num_of_procs); // ���� ������������ ��������� ��� ��������. ������ �������� ����� ������������ ��������� �����������
	MPI_Comm_rank(MPI_COMM_WORLD, &curr_rank_proc);

	// ������� ROOT �������� ���������������� ����� ���������
	if (curr_rank_proc == ROOT)
	{
		cout << "Enter the size:" << endl;
		cin >> size_arr;
		test_arr_seq_radix = Create_and_init_arr(size_arr);
		test_arr_pp_radix = new double[size_arr];
		for (int i = 0; i < size_arr; i++)
			test_arr_pp_radix[i] = test_arr_seq_radix[i];

		if (test_arr_seq_radix == NULL)
		{
			cout << "Incorrect input data, try again";
			return 0;
		}

		if (size_arr < MAX_SHOW_SIZE)
			Show_arr(test_arr_seq_radix, size_arr);

		// ������ ������ ���������������� ������

		seq_alg_time_start_radix = MPI_Wtime();
		LSDSortDouble(test_arr_seq_radix, size_arr);
		seq_alg_time_end_radix = MPI_Wtime();
		time_seq_work_alg_radix = seq_alg_time_end_radix - seq_alg_time_start_radix;

		if (size_arr < MAX_SHOW_SIZE)
		{
			cout << "Array sorted by sequence radix :" << endl;
			Show_arr(test_arr_seq_radix, size_arr);
		}

	}

	// ������������ ������ ������ ���������

	if (curr_rank_proc == ROOT)
		pp_alg_time_start_radix = MPI_Wtime();


	send_num_work = new int[num_of_procs];
	displs = new int[num_of_procs];

	/* ������������ ���������� � �������������� ������� ���������� ��������� ������� */

	MPI_Bcast(&size_arr, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
	if (curr_rank_proc > 0)
		test_arr_pp_radix = new double[size_arr];

	Calculate_work_and_displs(displs, send_num_work, size_arr);

	recv_buffer = new double[send_num_work[curr_rank_proc]];


	MPI_Scatterv(test_arr_pp_radix, send_num_work, displs, MPI_DOUBLE, recv_buffer, send_num_work[curr_rank_proc], MPI_DOUBLE, ROOT, MPI_COMM_WORLD);

	LSDSortDouble(recv_buffer, send_num_work[curr_rank_proc]);

	Sort_pp(recv_buffer, send_num_work);

	MPI_Gatherv(recv_buffer, send_num_work[curr_rank_proc], MPI_DOUBLE, test_arr_pp_radix, send_num_work, displs, MPI_DOUBLE, ROOT, MPI_COMM_WORLD);


	if (curr_rank_proc == ROOT)
	{
		pp_alg_time_end_radix = MPI_Wtime();

		time_pp_work_alg_radix = pp_alg_time_end_radix - pp_alg_time_start_radix;
	}

	if (curr_rank_proc == ROOT)
	{

		if (size_arr < MAX_SHOW_SIZE)
		{
			cout << "Array sorted by parallel radix :" << endl;
			Show_arr(test_arr_pp_radix, size_arr);
			cout << endl;
		}

		cout << "Sequence radix version worked:  " << time_seq_work_alg_radix << " seconds" << endl;
		cout << "Parallel radix worked: " << time_pp_work_alg_radix << " seconds" << endl;

		cout << endl;

		if (time_pp_work_alg_radix <= time_seq_work_alg_radix)
			cout << "Parallel version faster, than sequence version " << endl;
		else
			cout << "Sequence version faster, than parallel version " << endl;

		// ��������� ���������� �����������:
		

		//int good = true;
		//for (int i = 0; i < size_arr; i++)
		//	if (test_arr_pp_radix[i] != test_arr_seq_radix[i])// ���������� ���������� ���� ����������
		//	{
		//		cout << "Results parallel and sequence algorithm are not identical " << endl;
		//		good = false;
		//		break;
		//	}
		//if (good)
		//	cout << "Results parallel and sequence algorithm are identical " << endl;
		
		for (int i = 0; i <= size_arr; i++)
		{
			if (test_arr_pp_radix[i - 1] <= test_arr_pp_radix[i])
			{
				cout << "The results are correct!" << endl;
				break;
			}
		}

		delete[] test_arr_seq_radix;

	}
	
	
	delete[] test_arr_pp_radix;

	MPI_Finalize();

	return 0;
}