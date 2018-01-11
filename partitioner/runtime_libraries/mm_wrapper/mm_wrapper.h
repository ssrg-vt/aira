/*
 * Interface to register/unregister the wrappers.  Everything else wraps
 * C standard library functions, which are included via their respective
 * header files.
 */

#ifdef __cplusplus
extern "C" {
#endif

void register_mm_wrappers();
void unregister_mm_wrappers();
int get_size(void *ptr);
int get_size_np(void *ptr);
void *get_pointer(void *ptr);
void register_pointer(void *ptr, size_t size);
void unregister_pointer(void *ptr);

#ifdef __cplusplus
}
#endif

/*
 * Convenience macros
 */
#define INIT_MM_WRAPPER() register_mm_wrappers(); \
atexit(unregister_mm_wrappers)
