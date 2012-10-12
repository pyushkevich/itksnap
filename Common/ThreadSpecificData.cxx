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

#pragma warning("No support for WIN32 threads in ITK-SNAP yet!")

void ThreadSpecificDataSupport::Deleter(void *p)
{
  throw IRISException("ThreadSpecificData unsupported on Win32");
}

ThreadSpecificDataSupport::ThreadSpecificDataSupport()
{
  throw IRISException("ThreadSpecificData unsupported on Win32");
}

ThreadSpecificDataSupport::~ThreadSpecificDataSupport()
{
  throw IRISException("ThreadSpecificData unsupported on Win32");
}

void *ThreadSpecificDataSupport::GetPtrCreateIfNeeded(size_t data_size)
{
  throw IRISException("ThreadSpecificData unsupported on Win32");
  return NULL;
}

void *ThreadSpecificDataSupport::GetPtr() const
{
  throw IRISException("ThreadSpecificData unsupported on Win32");
  return NULL;
}


#else

#pragma warning("No support for non-pthread threads in ITK-SNAP yet!")

#endif

