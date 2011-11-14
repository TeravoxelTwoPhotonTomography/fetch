#pragma once
//Declaration

namespace fetch
{

  // Configurable
  // ============
  //
  // This class implements a behavior for communicating configuration
  // changes.
  //
  // Child objects get a pointer to a Config object.
  //
  // By default, the pointer addresses a default-constructed private instance.
  // However, it can be set to an outside instance via construction or
  // through the set_config() method.
  //
  // The set_config() method can be overloaded to intercept changes to
  // the <config> pointer and, for instance, communicate changes to other
  // objects.
  //
  // Aside
  // -----
  // The behavior is not specific to Configuration problems...it's more
  // really there just to provide a hook for a particular special property.
  // In fetch, it's used for configuration...hence the name.
  //
  // Usage
  // -----
  //
  // 1. Derive your class.
  // 
  //      class A: public Configurable<TConfig>
  //      ...
  //
  // 2. (optional) over-ride set_config().
  //
  //      void A::set_config(Config *cfg)
  //      { printf("changing config\n");
  //        config = cfg;
  //      }

  //template<typename T>
  //class ConfigurableBase
  //{
  // public: virtual void set_config(T *cfg) = 0;
  //};

  template<typename T>
  class Configurable//:public virtual ConfigurableBase<T>
  {
  public:
    typedef T Config;

    Config *_config;

    Configurable();
    Configurable(T *cfg);

    virtual void set_config(Config *cfg);      // returns 1 if updated, 0 otherwise
  private:
    Config _default_config;
  };


// Implementation

  template<typename T>
  Configurable<T>::Configurable()  
  : _config(0)
  {set_config(&_default_config);}

  template<typename T>
  Configurable<T>::Configurable( T *cfg )
  : _config(0)
  {set_config(cfg);}

  //************************************
  // Method:    set_config
  // FullName:  fetch::Configurable<T>::set_config
  // Access:    virtual public 
  // Returns:   void
  // Qualifier:
  // Parameter: Config * cfg
  //
  // Child classes might want to override this to communicate
  // config changes to other objects or parent classes.
  //
  //************************************
  template<typename T>
  void Configurable<T>::set_config( Config *cfg )
  { _config = cfg;
  }
}

