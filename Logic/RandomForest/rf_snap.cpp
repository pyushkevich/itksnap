#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define DEBUG_ON 0

// training data dimension N*D, NC is the number of catergory
#define N 5
#define D 3
#define NC 2
// testing data dimension TN*TD
#define TN 3
#define TD 3

void classRF(double *x, int *dimx, int *cl, int *ncl, int *cat, int *maxcat,
	     int *sampsize, int *strata, int *Options, int *ntree, int *nvar,
	     int *ipi, double *classwt, double *cut, int *nodesize,
	     int *outcl, int *counttr, double *prox,
	     double *imprt, double *impsd, double *impmat, int *nrnodes,
	     int *ndbigtree, int *nodestatus, int *bestvar, int *treemap,
	     int *nodeclass, double *xbestsplit, double *errtr,
	     int *testdat, double *xts, int *clts, int *nts, double *countts,
	     int *outclts, int labelts, double *proxts, double *errts,
         int *inbag, int print_verbose_tree_progression);

void classForest(int *mdim, int *ntest, int *nclass, int *maxcat,
        int *nrnodes, int *ntree, double *x, double *xbestsplit,
        double *pid, double *cutoff, double *countts, int *treemap,
        int *nodestatus, int *cat, int *nodeclass, int *jts,
        int *jet, int *bestvar, int *node, int *treeSize,
        int *keepPred, int *prox, double *proxMat, int *nodes);


int main(int argc, char *argv[])
{
	int i = 0;
	int j = 0;
	
	// input data starts here, must also set N, D, NC, TN, TD
	/* x_trn is the training data, consider as N*D matrix(array)
	   y_trn is the training data label, consider as N*1 vector(array), must be natural number start from 1
	   x_tst is the test data, dimension: TN*TD
	*/
	double *x_trn = (double*) calloc(N*D, sizeof(double));
	printf("training data:\n");
	for (i=0;i<N;i++)
	{
		for (j=0;j<D;j++)
		{
			if ((i == 0) || (i == 3))
			{
				x_trn[i*D+j] = 5;
			}
			else
			{
				x_trn[i*D+j] = 3;
			}
			printf("%f ", x_trn[i*D+j]);
		}
		printf("\n");
	}
	//int *y = (int*) calloc(N, sizeof(int));
	int y_trn[] = {1, 2, 2, 1, 2};
	printf("training data label:\n");
	for (i=0;i<N;i++)
	{
		printf("%d\n", y_trn[i]);
	}
	
	double x_tst[] = {3,3,3,5,5,5,3,3,3};
	printf("test data:\n");
	for (i=0;i<TN;i++)
	{
		for (j=0;j<TD;j++)
		{
			printf("%f ", x_tst[i*D+j]);
		}
		printf("\n");
	}
	
	
	// input data ends here
	
	
	//set default parameters
	int p_size = D;
	int n_size = N;
	int nclass = NC;
	
	int ntree = 500;
	int mtry = (int)(sqrt(p_size));
	
	int* cat = (int*) calloc(p_size,sizeof(int));
	
	for(i=0;i<p_size;i++)
	{
		cat[i]=1;
	}
	int maxcat = 1;
	
	int replace = 1;
	int* sampsize = (int*) calloc(1,sizeof(int));
	if (replace == 1)
	{
		*sampsize = N;
	}
	else
	{
		*sampsize = (int)(0.632 * N + 1);
	}
	
	int nsum = *sampsize;
	int* strata = (int*) calloc(1,sizeof(int));
	*strata = 1;
	
	int addclass = 0;
	int importance = 0;
	int localImp = 0;
	int proximity = addclass;
	int oob_prox = proximity;
	int do_trace = 0;
	if (DEBUG_ON)
	{
		do_trace = 1;
	}
	else
	{
		do_trace = 0;
	}
	int keep_forest = 1;
	int stratify = 0;
	int keep_inbag = 0;
	int Options[]={addclass,importance,localImp,proximity,oob_prox,do_trace,keep_forest,replace,stratify,keep_inbag};
	
	int nsample = 0;
	if (addclass)
	{
		nsample = 2 * n_size;
	}
	else
	{
		nsample = n_size;
	}
	
	int dimx[]={p_size, n_size};
	
	int tst_available = 0;
	
	int ipi = 0;
	double classwt[nclass];
	for(i = 0; i < nclass; i++)
	{
		classwt[i] = 1;
	}
	double cutoff[nclass];
	for(i = 0; i < nclass; i++)
	{
		cutoff[i] = 1.0 / (double)nclass;
	}
	int nodesize = 1;
	
	int print_verbose_tree_progression = 0;
	
	
	int nt = ntree;
	int* outcl = (int*) calloc(nsample,sizeof(int));
    int* counttr = (int*) calloc(nclass*nsample,sizeof(int));
	
	double* prox;
	if (proximity)
	{
		prox = (double*) calloc(nsample*nsample,sizeof(double));
	}
	else
	{
		prox = (double*) calloc(1,sizeof(double));
		prox[0]=1;
	}
		
	double* impout;
	double* impmat; 
	double* impSD;
	if (localImp)
	{
		if (addclass)
		{
			impmat = (double*) calloc(n_size*2*p_size,sizeof(double));
		}
		else
		{
			impmat = (double*) calloc(n_size*p_size,sizeof(double));
		}
	}
	else
	{
		impmat = (double*) calloc(1,sizeof(double));
		impmat[0] = 1;
	}
	
	if (importance)
	{
		impout=(double*) calloc(p_size*(nclass+2),sizeof(double));
		impSD =(double*) calloc(p_size*(nclass+1),sizeof(double));
	}
	else
	{
		impout=(double*) calloc(p_size,sizeof(double));
		impSD =(double*) calloc(1,sizeof(double));
	}
	
	int nrnodes = 2 * (int)(nsum / nodesize) + 1;
	
	int* ndbigtree = (int*) calloc(ntree,sizeof(int));
	int* nodestatus = (int*) calloc(nt*nrnodes,sizeof(int));
	int* bestvar = (int*) calloc(nt*nrnodes,sizeof(int));
	int* treemap = (int*) calloc(nt * 2 * nrnodes,sizeof(int));
	int* nodepred = (int*) calloc(nt * nrnodes,sizeof(int));
	double* xbestsplit = (double*) calloc(nt * nrnodes,sizeof(double));
	double* errtr = (double*) calloc((nclass+1) * ntree,sizeof(double));
	
	int* inbag;
	if (keep_inbag)
	{
		inbag = (int*) calloc(n_size*ntree,sizeof(int));
	}
	else
	{
		inbag = (int*) calloc(n_size,sizeof(int));
	}
	
	double *xts;
	int *yts;
	int nts;
	int *outclts;
	int labelts=0;
	double* proxts;
	double* errts;
	int testdat=0;
	double* countts;
	
	if (tst_available){
		/*
		xts = mxGetPr(prhs[19]);
		yts = (int*)mxGetData(prhs[20]);
		nts = (int)mxGetScalar(prhs[21]);
		plhs[19] = mxCreateNumericMatrix(nts, 1, mxINT32_CLASS, mxREAL);
		outclts = (int*)mxGetData(plhs[19]);
		countts = (double*) mxCalloc(nclass * nts,sizeof(double));
		if (proximity){
			plhs[20] = mxCreateNumericMatrix(nts, nts + n_size, mxDOUBLE_CLASS, mxREAL);
			proxts = (double*) mxGetData(plhs[20]); //calloc(nsample*nsample,sizeof(double));
		}else
		{
			plhs[20] = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
			proxts = (double*) mxGetData(plhs[20]); //calloc(1,sizeof(double));
			proxts[0]=1;
		}
		plhs[21] = mxCreateNumericMatrix((nclass+1), ntree, mxDOUBLE_CLASS, mxREAL);
		errts = (double*) mxGetData(plhs[21]); //calloc((nclass+1) * ntree,sizeof(double));
		labelts=1;
		testdat=1;
		*/
	}
	else
	{
		xts = (double*)malloc(sizeof(double)*1);
		yts = (int*)malloc(sizeof(int)*1);
		nts = 1;
		outclts = (int*) calloc(1,sizeof(int));
		countts = (double*) calloc(nclass * nts,sizeof(double));
		if (proximity)
		{
			proxts = (double*) calloc(1,sizeof(double));
		}
		else
		{
			proxts = (double*) calloc(1,sizeof(double));
			proxts[0]=1;
		}
		errts = (double*) calloc((nclass+1) * ntree,sizeof(double));
		labelts=0;
		testdat=0;
	}
	
	classRF(x_trn, dimx, y_trn, &nclass, cat, &maxcat,
		     sampsize, strata, Options, &ntree, &mtry,&ipi, 
	         classwt, cutoff, &nodesize,outcl, counttr, prox,
		     impout, impSD, impmat, &nrnodes,ndbigtree, nodestatus, 
	         bestvar, treemap,nodepred, xbestsplit, errtr,&testdat, 
	         xts, yts, &nts, countts,outclts, labelts, 
	         proxts, errts,inbag,print_verbose_tree_progression);
	
	
    // prediction starts here
	
	p_size = TD;
	n_size = TN;
	int mdim = p_size;
	
	nsum = n_size;
	addclass = 0;
	importance = 0;
	localImp = 0;
	oob_prox=0;
	do_trace=1;
	keep_forest=1;
	replace=1;
	stratify=0;
	keep_inbag=0;
	nt=ntree;
	ipi=0;
	nodesize=1;
	
	int* nodeclass = nodepred;
	
	testdat=0;
	int ntest = n_size;
	countts = (double*) calloc(nclass * ntest,sizeof(double));
	
//	int predict_all = 0;
    proximity = 1;
    int nodes = 1;
	
	int* jts;
	int* jet;
    int keepPred = 1;
	int* nodexts;
	
	if (nodes)
	{
		nodexts = (int*) calloc(ntest*ntree,sizeof(int));
	}
	else
	{
		nodexts = (int*) calloc(ntest,sizeof(int));
	}
	
	double *proxMat;
	
	if(proximity)
	{
		proxMat = (double*) calloc(ntest*ntest,sizeof(double));
	}
	else
	{
		proxMat = (double*) calloc(1,sizeof(double));
		proxMat[0]=1;
	}
	
	int* treeSize = ndbigtree;
	double* pid = classwt;
	
	jet = (int*) calloc(ntest,sizeof(int));
    
	if (keepPred)
	{
		jts = (int*) calloc(ntest*ntree,sizeof(int));
	}
	else
	{
		jts = (int*) calloc(ntest,sizeof(int));
	}
	
	classForest(&mdim, &ntest, &nclass, &maxcat,
        &nrnodes, &ntree, x_tst, xbestsplit,
        pid, cutoff, countts, treemap,
        nodestatus, cat, nodeclass, jts,
        jet, bestvar, nodexts, treeSize,
        &keepPred, &proximity, proxMat, &nodes);
	
	int * y_tst = (int*) calloc(ntest,sizeof(int));
	for (i = 0; i < ntest; i++)
	{
		y_tst[i] = jet[i];
	}
	
	double * votes = (double*) calloc(nclass * ntest,sizeof(double));
	double * probmap = (double*) calloc(nclass * ntest,sizeof(double));
	for (i = 0; i < ntest;i++)
	{
		for (j = 0; j < nclass; j++)
		{
			votes[i*nclass+j] = countts[i*nclass+j] / ntree;
			probmap[i*nclass+j] = votes[i*nclass+j] * 2.0 - 1.0;
		}
	}
	
	printf("classification for test data:\n");
	for (i = 0; i < ntest;i++)
	{
		printf("%d\n", y_tst[i]);
	}
	printf("votes:\n");
	for (i = 0; i < ntest;i++)
	{
		for (j = 0; j < nclass; j++)
		{
            printf("%f ", countts[i*nclass+j]);
		}
		printf("\n");
	}
	printf("probmap:\n");
	for (i = 0; i < ntest;i++)
	{
		for (j = 0; j < nclass; j++)
		{
            printf("%f ", votes[i*nclass+j]);
		}
		printf("\n");
	}
	printf("snap probmap:\n");
	for (i = 0; i < ntest;i++)
	{
		for (j = 0; j < nclass; j++)
		{
            printf("%f ", probmap[i*nclass+j]);
		}
		printf("\n");
	}
}
