#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
typedef struct thread_data
{
	int* a;
	int p, r;
}thread_data;
void merge(int* a, int p, int q, int r)
{
	int n1 = q - p + 1;
	int n2 = r - q;
	int* L = (int*)malloc(sizeof(int) * (n1 + 1));
	int* R = (int*)malloc(sizeof(int) * (n2 + 1));
	for (int i = 0; i < n1; i++)
		L[i] = a[p + i];
	for (int i = 0; i < n2; i++)
		R[i] = a[q + i + 1];
	L[n1] = 2147483647;
	R[n2] = 2147483647;
	int i = 0, j = 0;
	for (int k = p; k <= r; k++)
		if (L[i] < R[j])
		{
			a[k] = L[i];
			i++;
		}
		else
		{
			a[k] = R[j];
			j++;
		}
}
void p_merge_sort(thread_data *data)
{
	merge_sort(data->a, data->p, data->r);
}
void merge_sort(int *a, int p, int r)
{
	if (p >= r) return;
	pthread_t tid1, tid2;
	int q = (p + r) / 2;
	thread_data L_data, R_data;
	L_data.a = a;
	L_data.p = p;
	L_data.r = q;
	R_data.a = a;
	R_data.p = q + 1;
	R_data.r = r;
	pthread_create(&tid1, NULL, p_merge_sort, (void *) &L_data);
	pthread_create(&tid2, NULL, p_merge_sort, (void *) &R_data);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	merge(a, p, q, r);
}
int main()
{
	int n;
	printf("the array's size is: ");
	scanf("%d", &n);
	int* num = (int*)malloc(sizeof(int) * n);
	for (int i = 0; i < n; i++)
		scanf("%d", &num[i]);
	printf("the raw list is: ");
	for (int i = 0; i < n; i++)
		printf("%d ", num[i]);
	printf("\n");
	merge_sort(num, 0, n - 1);
	printf("the sorted list is: ");
	for (int i = 0; i < n; i++)
		printf("%d ", num[i]);
	printf("\n");
	return 0;
}
