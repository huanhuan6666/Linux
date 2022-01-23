#include<stdio.h>
#include<stdlib.h>
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
void merge_sort(int* a, int p, int r)
{
	if (p >= r) return;
	int q = (p + r) / 2;
	merge_sort(a, p, q);
	merge_sort(a, q + 1, r);
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
