#include "altrepisode.h"

#include <vector>

struct stdvec_double {

  // The altrep class that wraps a pointer to a std::vector<double>
  static R_altrep_class_t class_t;

  // Make an altrep object of class `stdvec_double::class_t`
  static SEXP Make(std::vector<double>* data, bool owner){
    Rprintf("[DEBUG] stdvec_double::Make called with data=%p, owner=%s\n", 
            data, owner ? "true" : "false");
    
    if (data) {
      Rprintf("[DEBUG] Vector size: %zu\n", data->size());
      if (!data->empty()) {
        Rprintf("[DEBUG] First element: %f\n", (*data)[0]);
      }
    }
    
    // The std::vector<double> pointer is wrapped into an R external pointer
    //
    // `xp` needs protection because R_new_altrep allocates
    SEXP xp = PROTECT(R_MakeExternalPtr(data, R_NilValue, R_NilValue));
    Rprintf("[DEBUG] Created external pointer: %p\n", xp);

    // If we own the std::vector<double>*, we need to delete it
    // when the R object is being garbage collected
    if (owner) {
      Rprintf("[DEBUG] Registering finalizer for owned vector\n");
      R_RegisterCFinalizerEx(xp, stdvec_double::Finalize, TRUE);
    }

    // make a new altrep object of class `stdvec_double::class_t`
    SEXP res = R_new_altrep(class_t, xp, R_NilValue);
    Rprintf("[DEBUG] Created altrep object: %p\n", res);

    // xp no longer needs protection, as it has been adopted by `res`
    UNPROTECT(1);
    Rprintf("[DEBUG] stdvec_double::Make returning\n");
    return res;
  }

  // finalizer for the external pointer
  static void Finalize(SEXP xp){
    std::vector<double>* ptr = static_cast<std::vector<double>*>(R_ExternalPtrAddr(xp));
    Rprintf("[DEBUG] stdvec_double::Finalize called for vector %p\n", ptr);
    if (ptr) {
      Rprintf("[DEBUG] Deleting vector of size %zu\n", ptr->size());
      delete ptr;
    } else {
      Rprintf("[DEBUG] WARNING: Finalizer called with null pointer\n");
    }
  }

  // get the std::vector<double>* from the altrep object `x`
  static std::vector<double>* Ptr(SEXP x) {
    SEXP data1 = R_altrep_data1(x);
    std::vector<double>* ptr = static_cast<std::vector<double>*>(R_ExternalPtrAddr(data1));
    Rprintf("[DEBUG] stdvec_double::Ptr returning %p\n", ptr);
    return ptr;
  }

  // same, but as a reference, for convenience
  static std::vector<double>& Get(SEXP vec) {
    Rprintf("[DEBUG] stdvec_double::Get called\n");
    std::vector<double>* ptr = Ptr(vec);
    if (!ptr) {
      Rprintf("[DEBUG] ERROR: Get() received null pointer!\n");
      // This will crash, but at least we'll know where
    }
    return *ptr;
  }

  // ALTREP methods -------------------

  // The length of the object
  static R_xlen_t Length(SEXP vec){
    Rprintf("[DEBUG] stdvec_double::Length called\n");
    R_xlen_t len = Get(vec).size();
    Rprintf("[DEBUG] Length returning: %ld\n", len);
    return len;
  }

  // What gets printed when .Internal(inspect()) is used
  static Rboolean Inspect(SEXP x, int pre, int deep, int pvec, void (*inspect_subtree)(SEXP, int, int, int)){
    Rprintf("[DEBUG] stdvec_double::Inspect called\n");
    Rprintf("std::vector<double> (len=%ld, ptr=%p)\n", Length(x), Ptr(x));
    return TRUE;
  }

  // ALTVEC methods ------------------

  // The start of the data, i.e. the underlying double* array from the std::vector<double>
  //
  // This is guaranteed to never allocate (in the R sense)
  static const void* Dataptr_or_null(SEXP vec){
    Rprintf("[DEBUG] stdvec_double::Dataptr_or_null called\n");
    const void* ptr = Get(vec).data();
    Rprintf("[DEBUG] Dataptr_or_null returning: %p\n", ptr);
    return ptr;
  }

  // same in this case, writeable is ignored
  static void* Dataptr(SEXP vec, Rboolean writeable){
    Rprintf("[DEBUG] stdvec_double::Dataptr called (writeable=%s)\n", 
            writeable ? "TRUE" : "FALSE");
    void* ptr = Get(vec).data();
    Rprintf("[DEBUG] Dataptr returning: %p\n", ptr);
    return ptr;
  }

  // ALTREAL methods -----------------

  // the element at the index `i`
  //
  // this does not do bounds checking because that's expensive, so
  // the caller must take care of that
  static double real_Elt(SEXP vec, R_xlen_t i){
    Rprintf("[DEBUG] stdvec_double::real_Elt called with index %ld\n", i);
    double val = Get(vec)[i];
    Rprintf("[DEBUG] real_Elt[%ld] = %f\n", i, val);
    return val;
  }

  // Get a pointer to the region of the data starting at index `i`
  // of at most `size` elements.
  //
  // The return values is the number of elements the region truly is so the caller
  // must not go beyond
  static R_xlen_t Get_region(SEXP vec, R_xlen_t start, R_xlen_t size, double* out){
    Rprintf("[DEBUG] stdvec_double::Get_region called (start=%ld, size=%ld)\n", 
            start, size);
    
    std::vector<double>& v = Get(vec);
    out = v.data() + start;
    R_xlen_t len = v.size() - start;
    R_xlen_t result = len > size ? size : len;  // Fixed the logic here
    
    Rprintf("[DEBUG] Get_region: vector_size=%zu, remaining=%ld, returning=%ld\n", 
            v.size(), len, result);
    
    return result;
  }

  // -------- initialize the altrep class with the methods above

  static void Init(DllInfo* dll){
    Rprintf("[DEBUG] stdvec_double::Init called\n");
    
    class_t = R_make_altreal_class("stdvec_double", "altrepisode", dll);
    Rprintf("[DEBUG] Created altrep class: %p\n", class_t);

    // altrep
    R_set_altrep_Length_method(class_t, Length);
    R_set_altrep_Inspect_method(class_t, Inspect);
    Rprintf("[DEBUG] Set altrep methods\n");

    // altvec
    R_set_altvec_Dataptr_method(class_t, Dataptr);
    R_set_altvec_Dataptr_or_null_method(class_t, Dataptr_or_null);
    Rprintf("[DEBUG] Set altvec methods\n");

    // altreal
    R_set_altreal_Elt_method(class_t, real_Elt);
    R_set_altreal_Get_region_method(class_t, Get_region);
    Rprintf("[DEBUG] Set altreal methods\n");
    
    Rprintf("[DEBUG] stdvec_double::Init completed successfully\n");
  }

};

// static initialization of stdvec_double::class_t
R_altrep_class_t stdvec_double::class_t;

// Called the package is loaded (needs Rcpp 0.12.18.3)
// [[Rcpp::init]]
void init_stdvec_double(DllInfo* dll){
  Rprintf("[DEBUG] init_stdvec_double called\n");
  stdvec_double::Init(dll);
  Rprintf("[DEBUG] init_stdvec_double completed\n");
}

//' an altrep object that wraps a std::vector<double>
//'
//' @export
// [[Rcpp::export]]
SEXP doubles() {
  Rprintf("[DEBUG] doubles() function called\n");
  
  // create a new std::vector<double>
  //
  // this uses `new` because we want the vector to survive
  // it is deleted when the altrep object is garbage collected
  auto v = new std::vector<double> {-2.0, -1.0, 0.0, 1.0, 2.0};
  
  Rprintf("[DEBUG] Created new vector at %p with %zu elements\n", v, v->size());
  for (size_t i = 0; i < v->size(); ++i) {
    Rprintf("[DEBUG] v[%zu] = %f\n", i, (*v)[i]);
  }

  // The altrep object owns the std::vector<double>
  SEXP result = stdvec_double::Make(v, true);
  Rprintf("[DEBUG] doubles() returning altrep object\n");
  return result;
}

template<class ForwardIt, class Generator>
void generate(ForwardIt first, ForwardIt last, Generator g){
    for (; first != last; ++first)
        *first = g();
}

// example C++ function that returns `n` random number between 0 and 1
std::vector<double> randoms(int n){
  Rprintf("[DEBUG] randoms(%d) called\n", n);
  std::vector<double> v(n);
  generate(begin(v), end(v), [] { return (double)std::rand() / RAND_MAX ; } );
  Rprintf("[DEBUG] Generated %d random numbers\n", n);
  return v;
}

// [[Rcpp::export]]
SEXP doubles_example(){
  Rprintf("[DEBUG] doubles_example() function called\n");
  
  // get a std::vector<double> from somewhere
  auto v = randoms(10);
  Rprintf("[DEBUG] Got vector with %zu elements\n", v.size());

  // wrap it into an altrep object of class `stdvec_double::class_t`
  // the altrep object does not own the vector, because we only need
  // it within this scope, it will be deleted just like any C++ object
  // at the end of this C++ function
  SEXP x = PROTECT(stdvec_double::Make(&v, false));
  Rprintf("[DEBUG] Created non-owning altrep object\n");

  // call sum(x) in the base environment
  SEXP s_sum = Rf_install("sum");
  Rprintf("[DEBUG] Got 'sum' symbol\n");
  
  SEXP call = PROTECT(Rf_lang2(s_sum, x));
  Rprintf("[DEBUG] Created call object\n");
  
  Rprintf("[DEBUG] About to evaluate sum() call\n");
  SEXP res = Rf_eval(call, R_BaseEnv);
  Rprintf("[DEBUG] sum() call completed\n");

  UNPROTECT(2);
  Rprintf("[DEBUG] doubles_example() returning\n");

  return res;
}
