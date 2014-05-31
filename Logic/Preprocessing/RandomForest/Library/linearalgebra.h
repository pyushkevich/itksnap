/**
 * Define some basic linear algebra operations.
 */

#ifndef LINEARALGEBRA_H
#define LINEARALGEBRA_H

typedef std::size_t size_t;
typedef std::size_t index_t;

template<class T>
void cholesky(const std::vector<std::vector<T> >& A,
              std::vector<std::vector<T> >& L)
{
  size_t N = A.size();
  for (index_t i = 0; i < N; ++i)
    {
      for (index_t j = 0; j < N; ++j)
        {
          if (j <= i)
            {
              T sum = 0;
              for (index_t k = 0; k < j; ++k)
                {
                  sum += L[i][k] * L[j][k];
                }
              L[i][j] = (i == j) ?
                    sqrt(A[i][i] - sum) :
                    (1.0 / L[j][j] * (A[i][j] - sum));
            }
          else
            {
              L[i][j] = 0;
            }
        }
    }
}

template<class T>
T spddeterminant(const std::vector<std::vector<T> >& A,
                 bool isTriangular = false)
{
  size_t N = A.size();
  T prod = 1;
  if (isTriangular)
    {
      for (index_t i = 0; i < N; ++i)
        {
          prod *= A[i][i] * A[i][i];
        }
      return prod;
    }
  else
    {
      std::vector<std::vector<T> > L;
      L.resize(N);
      for (index_t i = 0; i < N; ++i)
        {
          L[i].resize(N);
        }
      cholesky(A, L);
      for (index_t i = 0; i < N; ++i)
        {
          prod *= L[i][i] * L[i][i];
        }
      return prod;
    }
}

template<class T>
void multiply(const std::vector<std::vector<T> >& X,
              const std::vector<std::vector<T> >& Y,
              std::vector<std::vector<T> >& result)
{
  size_t rowXN = X.size();
  size_t colXN = X[0].size();
  size_t rowYN = Y.size();
  size_t colYN = Y[0].size();
  if (colXN != rowYN)
    {
      throw std::runtime_error("matrix multiplication: XColNum != YRowNum\n");
    }
  else
    {
      for (index_t i = 0; i < rowXN; ++i)
        {
          for (index_t j = 0; j < colYN; ++j)
            {
              T sum = 0;
              for (index_t k = 0; k < colXN; ++k)
                {
                  sum += X[i][k] * Y[k][j];
                }
              result[i][j] = sum;
            }
        }
    }
}

template<class T>
void multiplyXTX(const std::vector<std::vector<T> >& X,
                 std::vector<std::vector<T> >& result)
{
  size_t N = X.size();
  for (index_t i = 0; i < N; ++i)
    {
      for (index_t j = 0; j < N; ++j)
        {
          T sum = 0;
          for (index_t k = 0; k < N; ++k)
            {
              sum += X[k][i] * X[k][j];
            }
          result[i][j] = sum;
        }
    }
}

template<class T>
void trianginverse(const std::vector<std::vector<T> >& L,
                   std::vector<std::vector<T> >& invL)
{
  size_t N = L.size();
  for (index_t j = 0; j < N; ++j)
    {
      for (index_t i = 0; i < N; ++i)
        {
          if (j <= i)
            {
              T sum = (i == j)? 1 : 0;
              for (index_t k = j; k < i; ++k)
                {
                  sum -= L[i][k] * invL[k][j];
                }
              invL[i][j] = sum / L[i][i];
            }
          else
            {
              invL[i][j] = 0;
            }
        }
    }
}

template<class T>
void spdinverse(const std::vector<std::vector<T> >& A,
             std::vector<std::vector<T> >& invA,
             bool isTriangular = false)
{
  size_t N = A.size();
  static std::vector<std::vector<T> > invL;
  invL.resize(N);
  for (index_t i = 0; i < N; ++i)
    {
      invL[i].resize(N);
    }
  if (isTriangular)
    {
      trianginverse(A, invL);
    }
  else
    {
      std::vector<std::vector<T> > L;
      L.resize(N);
      for (index_t i = 0; i < N; ++i)
        {
          L[i].resize(N);
        }
      cholesky(A, L);
      trianginverse(L, invL);
    }
  multiplyXTX(invL, invA);
}

template<class T>
T multiplyXTspdAinvX(const std::vector<T>& X,
                     const std::vector<std::vector<T> >& A,
                     bool isTriangular = false)
{
  size_t N = A.size();
  if (N != X.size())
    {
      throw std::runtime_error("multiplyXTspdAinvX: XNum != ARowNum\n");
    }
  else
    {
      std::vector<std::vector<T> > invL;
      invL.resize(N);
      for (index_t i = 0; i < N; ++i)
        {
          invL[i].resize(N);
        }
      if (isTriangular)
        {
          trianginverse(A, invL);
        }
      else
        {
          std::vector<std::vector<T> > L;
          L.resize(N);
          for (index_t i = 0; i < N; ++i)
            {
              L[i].resize(N);
            }
          cholesky(A, L);
          trianginverse(L, invL);
        }
      std::vector<T> tmp(N, 0);
      for (index_t i = 0; i < N; ++i)
        {
          for (index_t j = 0; j < (i+1); ++j)
            {
              tmp[i] += invL[i][j] * X[j];
            }
        }
      T result = 0;
      for (index_t i = 0; i < N; ++i)
        {
          result += tmp[i] * tmp[i];
        }
      return result;
    }
}



#endif // LINEARALGEBRA_H
