#include <math.h>

static void gauss_solve(int n,double A[],double x[],double b[])
{
	int i,j,k,r;
	double max;
	for (k = 0; k < n-1; k++)	{
		max = fabs(A[k*n+k]); /*find maxmum*/
		r = k;
		for (i = k + 1; i < n-1; i++) {
			if (max < fabs(A[i*n+i])) {
				max = fabs(A[i*n+i]);
				r=i;
			}
		}

		if (r != k) {
			for (i = 0; i < n; i++) /*change array:A[k]&A[r] */	{
				max = A[k*n+i];
				A[k*n+i] = A[r*n+i];
				A[r*n+i] = max;
			}
		}

		max = b[k]; /*change array:b[k]&b[r] */
		b[k] = b[r];
		b[r] = max;
		for (i = k+1; i<n; i++)	{
			for (j = k+1; j < n; j++)
				A[i*n+j] -= A[i*n+k] * A[k*n+j] / A[k*n+k];
			b[i] -= A[i*n+k] * b[k] / A[k*n+k];
		}
	}

	for (i = n-1; i >= 0; x[i] /= A[i*n+i], i--) {
		for (j = i+1, x[i] = b[i]; j<n; j++) {
			x[i] -= A[i*n+j] * x[j];
		}
	}
}

//*==================polyfit(n,x,y,poly_n,a)===================*/
//*=======���y=a0+a1*x+a2*x^2+����+apoly_n*x^poly_n========*/
//*=====n�����ݸ��� xy������ֵ poly_n�Ƕ���ʽ������======*/
//*===����a0,a1,a2,����a[poly_n]��ϵ����������һ�������=====*/

void polyfit(int n, double x[], double y[], int poly_n, double a[])
{
	int i,j;
	double *tempx, *tempy, *sumxx, *sumxy, *ata;

	tempx = new double[n];
	sumxx = new double[poly_n*2+1];
	tempy = new double[n];
	sumxy = new double[poly_n+1];
	ata = new double[(poly_n+1)*(poly_n+1)];

	/**	x,y Ϊ���������ݣ�n ��Ŀ���� poly_n+1��

		�൱����� poly_n + 1 ���⣬���� n ������
		Ax=y 
		
		��������ܿ���û��ķ��̣����������Ľ��ƽ��أ��������Դ�����˼ά��˼����˼·�ǳ�ֱ�ۡ�����֪���������п��ܵ�����x��Ax��ȡֵ
		������һ�������ռ䣬��������A���пռ䣬A���пռ��е������������Ǿ���A�������������Ժ͡�(ע�����Կ���Ax�ų��걸�ռ��е�һ���ӿ�
		�䣬����ƽ��;��൱����y�ڸ��ӿռ��е�ͶӰ�Ľ⣩����Ax=y�޽⵱�ҽ�������y����A���пռ��С���Ȼy����A���пռ䣬Ҫ��ý��ƽ⣬
		����Ȼ���뷨����A���пռ����ҵ�һ����y����ĵ�y'�������̱�Ϊ��

			Ax = y'

		���弸��֪ʶ�������ǣ�����ƽ�����һ��p��ƽ������������ĵ㣬�ǵ�p�ڸ�ƽ���ϵ�����ͶӰ�㡣��ͨ����p��ƽ�������ߣ�������ƽ��Ľ�
		�㣬��Ϊ��ƽ�������p����ĵ㡣��������ڸ�ά�ռ�����Ȼ����������(y-y')�ǵ�y���y'�����ߣ����Ը�������A���пռ䴹ֱ����������
		���Ա����ǣ�

			��A���пռ��е���������Av��(Av)T(y-y') = 0

			����һ�£�vTAT(y-y') = 0��

			����v������������һ��������������˶�����0����������Ȼ������������һ�¸��������Լ���ˣ������ȻΪ0��������AT(y-y')=0��
			��ʵ�ϣ������A���ĸ������ӿռ�Ƚ��˽�Ļ������ʽ�ӵĵ��������ֱ�ӣ���ΪA���пռ���AT����ռ䣨��ATx=0�Ľ�ռ䣩Ϊ����
			�����ӿռ䣬������A���пռ���������������AT����ռ��У�����AT(y-y')=0��

		��y' = Ax���벢����

			ATAx = ATy

		��Ax=y�޽�ʱ�������ʽ�������ܵõ�x�Ľ��ƽ⣬�������������С���˷���

		���磬����ƽ���������������ߵĵ�(1,1)��(2,2)��(3,2)��Ҫ���һ�����ƴ������������ֱ��y=c1+c2x��������ⷽ�̣�

			XTXc=XTy

		����X=[1 1; 1 2; 1 3]��y=[1;2;2]
		���Խ��ϵ������c = (XTX)-1XTy = [0.6667;0.5]

		����ʽ��ϣ�������Ϊ��һ�鷽�̣���� a0, a1, a2 ... 
		y0 = a0 + a1x1 + a2x1*x1 + a3x1*x1*x1 + ...
					||
					|| 
					|| 
					\/  
			y   =   A                    a

				  [ 1 x1 x1^2 x1^3 ... ][a0]
 			[y] = [ 1 x2 x2^2 x2^3 ... ][a1]
				  [ .......            ][.]
				  
			���� [y] [x1, x2, ...] ��֪����� [a0, a1, ...]

			��Ϊ ϵ�������п��ܲ����棬���涼����ϵ�������ת��
				ATy = ATAa
			ATA ��������
				ATAa = ATy	��� a ����
	 */

	for (i = 0; i < n; i++) {
		tempx[i] = 1;
		tempy[i] = y[i];
	}

	for (i = 0; i < 2*poly_n + 1; i++) {
		sumxx[i] = 0;
		for (j = 0; j < n; j++) {
			sumxx[i] += tempx[j];
			tempx[j] *= x[j];
		}
	}

	for (i = 0; i < poly_n+1; i++) {
		sumxy[i] = 0;
		for (j = 0; j < n; j++) {
			sumxy[i] += tempy[j];
			tempy[j] *= x[j];
		}
	}

	// ATA
	for (i = 0; i < poly_n+1; i++) {
		for (j = 0; j < poly_n+1; j++) {
			ata[i * (poly_n + 1) + j] = sumxx[i + j];
		}
	}

	gauss_solve(poly_n+1, ata , a, sumxy);

	delete []tempx;
	delete []sumxx;
	delete []tempy;
	delete []sumxy;
	delete []ata;
}
