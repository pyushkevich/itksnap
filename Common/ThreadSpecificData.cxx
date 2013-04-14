#include "ThreadSpecificData.h"
#include "itkMultiThreader.h"
#include "IRISException.h"

#if defined(ITK_USE_PTHREADS)

void ThreadSpecificDataSupport::Deleter(void *p)
{
  free(p);
}

ThreadSpecificDataSupport::ThreadSpecificDataSupport()
{
  // Allocate storage for a key
  pthread_key_t *pkey = new pthread_key_t[1];

  // Create the key
  int rc = pthread_key_create(pkey, &ThreadSpecificDataSupport::Deleter);
  if(rc)
    throw IRISException("pthread_key_create failed with rc = %d", rc);

  // Store the key
  m_KeyPointer = pkey;
}

ThreadSpecificDataSupport::~ThreadSpecificDataSupport()
{
  // Delete the key
  pthread_key_t *pkey = (pthread_key_t *) m_KeyPointer;
  int rc = pthread_key_delete(pkey[0]);
  if(rc)
    throw IRISException("pthread_key_delete failed with rc = %d", rc);
}

void *ThreadSpecificDataSupport::GetPtrCreateIfNeeded(size_t data_size)
{
  pthread_key_t *pkey = (pthread_key_t *) m_KeyPointer;
  void *pdata = pthread_getspecific(pkey[0]);
  if(!pdata)
    {
    pdata = malloc(data_size);
    int rc  = pthread_setspecific(pkey[0], pdata);
    if(rc)
      throw IRISException("pthread_setspecific failed with rc = %d", rc);
    }
  return pdata;
}

void *ThreadSpecificDataSupport::GetPtr() const
{
  pthread_key_t *pkey = (pthread_key_t *) m_KeyPointer;
  void *pdata = pthread_getspecific(pkey[0]);
  return pdata;
}

#elif defined(ITK_USE_WIN32_THREADS)

void ThreadSpecificDataSupport::Deleter(void *p)
{
}

ThreadSpecificDataSupport::ThreadSpecificDataSupport()
{
  DWORD *key = new DWORD[1];
  key[0] = TlsAlloc();
  if(key[0] == TLS_OUT_OF_INDEXES)
    throw IRISException("TlsAlloc failed with error %d", GetLastError());
  m_KeyPointer = key;
}

ThreadSpecificDataSupport::~ThreadSpecificDataSupport()
{
  DWORD *key = (DWORD *) m_KeyPointer;
  
  // Delete the stored stuff
  void *pdata = TlsGetValue(key[0]);
  if(pdata)
    free(pdata);

  TlsFree(key[0]);
  delete [] key;
}

void *ThreadSpecificDataSupport::GetPtrCreateIfNeeded(size_t data_size)
{
  DWORD *key = (DWORD *) m_KeyPointer;
  void *pdata = TlsGetValue(key[0]);

  if(!pdata)
    {
    if(GetLastError() != ERROR_SUCCESS)
      throw IRISException("TlsGetValue failed with error %d", GetLastError());

    pdata = malloc(data_size);
    if(!TlsSetValue(key[0], pdata))
      throw IRISException("TlsSetValue failed with error %d", GetLastError());
    }

  return pdata;
}

void *ThreadSpecificDataSupport::GetPtr() const
{
  DWORD *key = (DWORD *) m_KeyPointer;
  void *pdata = TlsGetValue(key[0]);
  return pdata;
}


#else

#pragma warning("No support for non-pthread threads in ITK-SNAP yet!")

#endif

