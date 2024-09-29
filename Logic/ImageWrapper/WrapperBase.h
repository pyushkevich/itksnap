#ifndef WRAPPERBASE_H
#define WRAPPERBASE_H

#include "SNAPCommon.h"
#include "SNAPEvents.h"
#include "itkObject.h"
#include "TagList.h"

class AbstractDisplayMappingPolicy;
class TDigestDataObject;

class WrapperBase : public itk::Object
{
public:
  irisITKAbstractObjectMacro(WrapperBase, itk::Object)

  //------------------------------------------
  //  Begin virtual methods definition

  /**
   * Get the display mapping policy. This policy differs from wrapper to wrapper
   * and may involve using color labels or color maps.
   */
  virtual AbstractDisplayMappingPolicy *GetDisplayMapping() = 0;

  /**
   * Get the display mapping policy. This policy differs from wrapper to wrapper
   * and may involve using color labels or color maps.
   */
  virtual const AbstractDisplayMappingPolicy *GetDisplayMapping() const = 0;

  /** Access the filename */
  irisVirtualGetStringMacro(FileName)
  irisVirtualSetStringMacro(FileName)

  /**
   * Access the nickname - which may be a custom nickname or derived from the
   * filename if there is no custom nickname
   */
  irisVirtualGetMacro(Nickname, const std::string &)

  // Set the custom nickname - precedence over the filename
  irisVirtualGetMacro(CustomNickname, const std::string &)
  irisVirtualSetMacro(CustomNickname, const std::string &)

  /** Fallback nickname - shown if no filename and no custom nickname set. */
  irisVirtualGetMacro(DefaultNickname, const std::string &)
  irisVirtualSetMacro(DefaultNickname, const std::string &)

  /** List of tags assigned to the image layer */
  irisVirtualGetMacro(Tags, const TagList &)
  irisVirtualSetMacro(Tags, const TagList &)

  /** Layer transparency */
  irisVirtualSetMacro(Alpha, double)
  irisVirtualGetMacro(Alpha, double)

  /**
   * Is the image initialized?
   */
  irisVirtualIsMacro(Initialized)

  /**
   * The wrappers has a generic mechanism for associating data with it.
   * For example, we can associate some parameter values for a specific
   * image processing algorithm with each layer. To do that, we simply
   * assign a pointer to the data to a specific string role. Internally,
   * a smart pointer is used to point to the associated data.
   *
   * Users of this method might also want to rebroadcast events from the
   * associated object as events of type WrapperUserChangeEvent(). These
   * events will then propagate all the way up to the IRISApplication.
   *
   * It is implemented in this base class, since the method is generic
   * enough to be used by almost all subclasses. Subclass can always override
   * this implementation for wrapper specific logic
   *
   */
  virtual void SetUserData(const std::string &role, itk::Object *data);

  /**
   * Get the user data associated with this wrapper for a specific role. If
   * no association exists, NULL is returned.
   */
  virtual itk::Object* GetUserData(const std::string &role) const;


  virtual void RemoveUserData(const std::string &role)
  {
    if (m_UserDataMap.count(role))
      m_UserDataMap.erase(role);
  }

  /**
    Obtain the t-digest, from which the quantiles of the image/mesh can be
    approximated. The t-digest algorithm by T. Dunning is a fast algorithm for
    approximating statistics such as quantiles, histogram, CDF, etc. It replaces
    earliercode that separately computed the image or mesh min/max and histogram using
    separate filters and required the number of histogram bins to be specified explicitly.

    t-digest code: https://github.com/SpirentOrion/digestible
    t-digest paper: https://www.sciencedirect.com/science/article/pii/S2665963820300403
    */
  virtual TDigestDataObject *GetTDigest() = 0;

  //  End of virtual methods
  //------------------------------------------

  /**
    Get a unique id for this wrapper. All wrappers ever created have
    different ids.
    */
  unsigned long GetUniqueId() const;

protected:
  WrapperBase();
  virtual ~WrapperBase() = default;

  // A map to store user-associated data
  typedef std::map<std::string, SmartPtr<itk::Object> > UserDataMapType;
  UserDataMapType m_UserDataMap;

  unsigned long m_UniqueId;

};

#endif // WRAPPERBASE_H
