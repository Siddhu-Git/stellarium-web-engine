
/*
 * Function: module_list_obj
 * List all astro objects in a module.
 *
 * Parameters:
 *   module   - The module (core for all objects).
 *   obs      - The observer used to compute the object vmag.
 *   max_mag  - Only consider objects below this magnitude.
 *   user     - Data passed to the callback.
 *   f        - Callback function called once per object.
 *
 * Return:
 *    0         - Success.
 *   -1         - The object doesn't support listing, or a hint is needed.
 *   OBJ_AGAIN  - Some resources are still loading and so calling the function
 *                again later might return more values.
 */
int module_list_objs(const obj_t *module, observer_t *obs,
                     double max_mag, uint64_t hint, void *user,
                     int (*f)(void *user, obj_t *obj));

/*
 * Function: module_add_data_source
 * Add a data source url to a module
 *
 * Parameters:
 *   module - a module, or NULL for any module.
 *   url    - base url of the data.
 *   type   - type of data.  NULL for directory.
 *   args   - additional arguments passed.  Can be used by the modules to
 *            check if they can handle the source or not.
 *
 * Return:
 *   0 if the source was accepted.
 *   1 if the source was no recognised.
 *   a negative error code otherwise.
 */
int module_add_data_source(obj_t *module, const char *url, const char *type,
                           json_value *args);

/*
 * Function: module_add_sub
 * Create a dummy sub object
 *
 * This can be used in the obj_call function to Assign attributes to sub
 * object.  For example it can be used to have constellations.images, without
 * having to create a special class just for that.
 */
obj_t *module_add_sub(obj_t *module, const char *name);


//XXX: probably should rename this to obj_query.
/*
 * Function: obj_get
 * Find an object by query.
 *
 * Parameters:
 *   module - The parent module we search from, NULL for all modules.
 *   query  - An identifier that represents the object, can be:
 *      - A direct object id (HD 456, NGC 8)
 *      - A module name (constellations)
 *      - A submodule (constellations.lines)
 *      - An object name (polaris)
 *   flags  - always zero for the moment.
 */
obj_t *obj_get(const obj_t *module, const char *query, int flags);

/*
 * Function: obj_get_by_oid
 * Find an object by its oid.
 */
obj_t *obj_get_by_oid(const obj_t *module, uint64_t oid, uint64_t hint);

/*
 * Function: obj_get_by_nsid
 * Find an object by its nsid.
 */
obj_t *obj_get_by_nsid(const obj_t *module, uint64_t nsid);

/*
 * Function: module_get_render_order
 *
 * For modules: return the order in which the modules should be rendered.
 * NOTE: if we used deferred rendering this wouldn't be needed at all!
 */
double module_get_render_order(const obj_t *module);

/*
 * Function: module_add_global_listener
 * Register a callback to be called anytime an attribute of a module changes.
 *
 * For the moment we can only have one listener for all the modules.  This
 * is enough for the javascript binding.
 */
void module_add_global_listener(void (*f)(obj_t *module, const char *attr));

/*
 * Function: module_changed
 * Should be called by modules after they manually change one of their
 * attributes.
 */
void module_changed(obj_t *module, const char *attr);

/*
 * Macro: MODULE_ITER
 * Iter all the children of a given module of a given type.
 *
 * Properties:
 *   module - The module we want to iterate.
 *   child  - Pointer to an object, that will be set with each child.
 *   klass_ - Children klass type id string, or NULL for no filter.
 */
#define MODULE_ITER(module, child, klass_) \
    for (child = (void*)(((obj_t*)module)->children); child; \
                        child = (void*)(((obj_t*)child)->next)) \
        if (!(klass_) || \
                ((((obj_t*)child)->klass) && \
                 (((obj_t*)child)->klass->id) && \
                 (strcmp(((obj_t*)child)->klass->id, klass_ ?: "") == 0)))

/*
 * Function: module_add
 * Add an object as a child of a module
 */
void module_add(obj_t *module, obj_t *child);

/*
 * Function: module_remove
 * Remove an object from a parent.
 */
void module_remove(obj_t *module, obj_t *child);