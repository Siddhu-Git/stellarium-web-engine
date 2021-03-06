/* Stellarium Web Engine - Copyright (c) 2018 - Noctua Software Ltd
 *
 * This program is licensed under the terms of the GNU AGPL v3, or
 * alternatively under a commercial licence.
 *
 * The terms of the AGPL v3 license can be found in the main directory of this
 * repository.
 */

// File: core.h

#include "areas.h"
#include "frames.h"
#include "labels.h"
#include "projection.h"
#include "skyculture.h"
#include "hips.h"
#include "observer.h"
#include "obj.h"
#include "module.h"
#include "otypes.h"
#include "telescope.h"
#include "tonemapper.h"

#include "utils/fps.h"

typedef struct core core_t;
typedef struct task task_t;

extern core_t *core;    // Global core object.

/******* Section: Core ****************************************************/

/*
 * Type: task_t
 * Contains info about some extra running tasks.
 *
 * All the tasks will be called once before the module update.  A task runs
 * as long as it returns zero.
 */
struct task
{
    task_t *next, *prev;
    int (*fun)(task_t *task, double dt);
    void *user;
};

/* Type: core_t
 * Contains all the modules and global state of the program.
 */
struct core
{
    obj_t           obj;
    observer_t      *observer;
    double          fov;

    // Two parameters to manually adjust the size of the stars.
    double          star_linear_scale;
    double          star_scale_screen_factor;
    double          star_relative_scale;

    // Bortle index, see https://en.wikipedia.org/wiki/Bortle_scale
    int             bortle_index;

    // Objects fainter than this magnitude won't be displayed, independently
    // of zoom/exposure levels. Set to e.g. 99 to practically disable.
    double          display_limit_mag;

    tonemapper_t    tonemapper;
    bool            fast_adaptation; // True if eye adpatation is fast
    double          tonemapper_p;
    double          lwmax; // Max visible luminance.
    double          lwmax_min; // Min value for lwmax.
    double          lwsky_average;  // Current average sky luminance
    double          max_point_radius; // Max radius in pixel.
    double          min_point_radius;
    double          skip_point_radius;
    double          show_hints_radius; // Min radius to show stars labels.

    telescope_t     telescope;
    bool            telescope_auto; // Auto adjust telescope.
    double          exposure_scale;

    bool            flip_view_vertical;
    bool            flip_view_horizontal;

    renderer_t      *rend;
    int             proj;
    double          win_size[2];
    double          win_pixels_scale;
    obj_t           *selection;
    obj_t           *hovered;

    fps_t           fps; // FPS counter.

    // Number of clicks so far.  This is just so that we can wait for clicks
    // from the ui.
    int clicks;
    bool ignore_clicks; // Don't select on click.

    struct {
        struct {
            int    id; // Backend id (for example used in js).
            double pos[2];
            bool   down[2];
        } touches[2];
        bool        keys[512]; // Table of all key state.
        uint32_t    chars[16]; // Unicode characters.
    } inputs;
    bool            gui_want_capture_mouse;

    struct {
        obj_t       *lock;    // Optional obj we lock to.
        double      src_q[4]; // Initial pos quaternion.
        double      dst_q[4]; // Destination pos quaternion.
        double      t;        // Goes from 0 to 1 as we move.
        double      duration; // Animation duration in sec.
        // Set to true if the move is toward newly locked object.
        bool        move_to_lock;
    } target;

    struct {
        double      t;        // Goes from 0 to 1 as we move.
        double      duration; // Animation duration in sec.
        double      src_fov;  // Initial fov.
        double      dst_fov;  // Destination fov.
    } fov_animation;

    struct {
        double      t;        // Goes from 0 to 1.
        double      duration; // Animation duration in sec.
        double      src;
        double      dst;
    } time_animation;

    // Zoom movement. -1 to zoom out, +1 to zoom in.
    double zoom;

    // Maintains a list of clickable/hoverable areas.
    areas_t         *areas;

    // FRAME_OBSERVED for altaz mount.
    int mount_frame;

    // Click callback that can be set by the client.  If it returns true,
    // the event is canceled (no selection is made).
    bool (*on_click)(double x, double y);

    // List of running tasks.
    task_t *tasks;

    // Can be used for debugging.  It's conveniant to have an exposed test
    // attribute.
    bool test;
};

enum {
    KEY_ACTION_UP      = 0,
    KEY_ACTION_DOWN    = 1,
    KEY_ACTION_REPEAT  = 2,
};

// Key id, same as GLFW for convenience.
enum {
    KEY_ESCAPE      = 256,
    KEY_ENTER       = 257,
    KEY_TAB         = 258,
    KEY_BACKSPACE   = 259,
    KEY_DELETE      = 261,
    KEY_RIGHT       = 262,
    KEY_LEFT        = 263,
    KEY_DOWN        = 264,
    KEY_UP          = 265,
    KEY_PAGE_UP     = 266,
    KEY_PAGE_DOWN   = 267,
    KEY_HOME        = 268,
    KEY_END         = 269,
    KEY_SHIFT       = 340,
    KEY_CONTROL     = 341,
};

void core_init(double win_w, double win_h, double pixel_scale);

void core_release(void);

/*
 * Function: core_update
 * Update the core and all the modules
 *
 * Parameters:
 *   dt     - Time imcrement from last frame (sec).
 */
int core_update(double dt);

/*
 * Function: core_update_fov
 * Update the core fov animation.
 *
 * Should be called before core_update
 *
 * Parameters:
 *   dt     - Time imcrement from last frame (sec).
 */
void core_update_fov(double dt);

/*
 * Function: core_observer_update
 * Update the observer.
 */
void core_observer_update();

/*
 * Function: core_set_view_offset
 * Update the view center vertical offset.
 *
 * Call this e.g. when a panel use the bottom or upper part of the screen,
 * and you want to have the zoom center at the center of the remaining sky
 * screen space.
 */
void core_set_view_offset(double center_y_offset);

int core_render(double win_w, double win_h, double pixel_scale);
// x and y in screen coordinates.
void core_on_mouse(int id, int state, double x, double y);
void core_on_key(int key, int action);
void core_on_char(uint32_t c);
void core_on_zoom(double zoom, double x, double y);

/*
 * Function: core_on_pinch
 * Called from the cliend to perform a pinch/panning gesture.
 *
 * Parameters:
 *   state  - State of the panning gesture:
 *              0 - Panning started.
 *              1 - Panning updated.
 *              2 - Panning ended.
 *   x      - X position in windows coordinates.
 *   y      - Y position in windows coordinates.
 *   scale  - Pinch scale (starts at 1).
 */
void core_on_pinch(int state, double x, double y, double scale);

/*
 * Function: core_get_proj
 * Get the core current view projection
 *
 * Parameters:
 *   proj   - Pointer to a projection_t instance that get initialized.
 */
void core_get_proj(projection_t *proj);

/*
 * Function: core_get_obj_at
 * Get the object at a given screen position.
 *
 * Parameters:
 *   x        - The screen x position.
 *   y        - The screen y position.
 *   max_dist - Maximum distance in pixel between the object and the given
 *              position.
 */
obj_t *core_get_obj_at(double x, double y, double max_dist);

/*
 * Function: core_get_module
 * Return a core module by name
 *
 * Parameter:
 *    id    - Id or dot separated path to a module.  All the modules have
 *            the path 'core.<something>', but to make it simpler, here
 *            we also accept to search without the initial 'core.'.
 *
 * Return:
 *    The module object, or NULL if none was found.
 */
obj_t *core_get_module(const char *id);

/*
 * Function: core_report_vmag_in_fov
 * Inform the core that an object with a given vmag is visible.
 *
 * This is used for the eyes adaptations algo.
 *
 * Parameters:
 *   vmag - The magnitude of the object.
 *   r    - Visible radius of the object (rad).
 *   sep  - Separation of the center of the object to the center of the
 *          screen.
 */
void core_report_vmag_in_fov(double vmag, double r, double sep);

void core_report_luminance_in_fov(double lum, bool fast_adaptation);

/*
 * Function: core_get_point_for_mag
 * Compute a point radius and luminosity from a visual magnitude.
 *
 * Parameters:
 *   mag       - The visual magnitude.
 *   radius    - Output radius in window pixels.
 *   luminance - Output luminance from 0 to 1, gamma corrected.  Ignored if
 *               set to NULL.
 */
bool core_get_point_for_mag(double mag, double *radius, double *luminance);

/*
 * Function: core_mag_to_illuminance
 * Compute the illuminance for a given magnitude.
 *
 * This function is independent from the object surface area.
 *
 * Parameters:
 *   mag       - The visual magnitude integrated over the object's surface.
 *
 * Return:
 *   Object illuminance in lux (= lum/m² = cd.sr/m²)
 */
double core_mag_to_illuminance(double vmag);

/*
 * Function: core_mag_to_surf_brightness
 * Compute the sufrace brightness from a mag and surface.
 *
 * Parameters:
 *   mag       - The object's visual magnitude.
 *   surf      - The object's angular surface in rad^2
 *
 * Return:
 *   Object surface brightness in mag/arcsec²
 */
double core_mag_to_surf_brightness(double mag, double surf);

/*
 * Function: core_illuminance_to_lum_apparent
 * Compute the apparent luminance from an object's luminance and surface.
 *
 * Parameters:
 *   illum     - The illuminance.
 *   surf      - The angular surface in rad^2
 *
 * Return:
 *   Object luminance in cd/m².
 */
double core_illuminance_to_lum_apparent(double illum, double surf);

/*
 * Function: core_surf_brightness_to_lum_apparent
 * Compute the apparent luminance from an objet's surface brightness.
 *
 * Parameters:
 *   surf_brightness - The object surface brightness in mag/arcsec²
 *
 * Return:
 *   Object apparent luminance in cd/m².
 */
double core_surf_brightness_to_lum_apparent(double surf_brightness);

/*
 * Function: core_mag_to_lum_apparent
 * Compute the apparent luminance from an objet's magnitude and surface.
 *
 * Parameters:
 *   mag       - The visual magnitude integrated over the object's surface.
 *   surf      - The angular surface in rad^2
 *
 * Return:
 *   Object luminance in cd/m².
 */
double core_mag_to_lum_apparent(double mag, double surf);

/*
 * Function: core_get_apparent_angle_for_point
 * Get angular radius of a round object from its pixel radius on screen.
 *
 * For example this can be used after core_get_point_for_mag to estimate the
 * angular size a circle should have to exactly fit the object.
 *
 * Parameters:
 *   proj   - The projection used.
 *   r      - Radius on screen in window pixels.
 *
 * Return:
 *   Angular radius in radian.  This is the physical radius, not scaled by
 *   the fov.
 *
 * XXX: make this a function of the projection directly?
 */
double core_get_apparent_angle_for_point(const projection_t *proj, double r);

/*
 * Function: core_get_point_for_apparent_angle
 * Get the pixel radius of a circle with a given apparent angle
 *
 * This is the inverse of <core_get_apparent_angle_for_point>
 *
 * Parameters:
 *   proj   - The projection used.
 *   angle  - Apparent angle (rad).
 *
 * Return:
 *   Point radius in window pixel.
 */
double core_get_point_for_apparent_angle(const projection_t *proj,
                                         double angle);

/*
 * Function: core_lookat
 * Move view direction to the given position.
 *
 * For example this can be used after core_get_point_for_mag to estimate the
 * angular size a circle should have to exactly fit the object.
 *
 * Parameters:
 *   pos      - The wanted pointing 3D direction in the OBSERVED frame.
 *   duration - Movement duration in sec.
 */
void core_lookat(const double *pos, double duration);

/*
 * Function: core_point_and_lock
 * Move view direction to the given object and lock on it.
 *
 * Parameters:
 *   target   - The target object.
 *   duration - Movement duration in sec.
 */
void core_point_and_lock(obj_t *target, double duration);

/*
 * Function: core_zoomto
 * Change FOV to the passed value.
 *
 * Parameters:
 *   fov      - The target FOV diameter in rad.
 *   duration - Movement duration in sec.
 */
void core_zoomto(double fov, double duration);

/*
 * Function: core_set_time
 * Change the core observer time, possibly using an animation.
 *
 * Parameters:
 *   tt       - The target time in TT MJD.
 *   duration - Transition duration in sec.
 */
void core_set_time(double tt, double duration);

// Return a static string representation of an object type id.
const char *otype_to_str(const char *type);

// Create or get a city.
obj_t *city_create(const char *name, const char *country_code,
                   const char *timezone,
                   double latitude, double longitude,
                   double elevation,
                   double get_near);

/*
 * Function: skycultures_get_label
 * Get the label of a sky object in the current skyculture, translated
 * for the current language.
 *
 * Parameters:
 *   main_id        - the main ID of the sky object:
 *                     - for bright stars use "HIP XXXX"
 *                     - for constellations use "CON culture_name XXX"
 *                     - for planets use "NAME Planet"
 *                     - for DSO use the first identifier of the names list
 *   out            - A text buffer that get filled with the name.
 *   out_size       - size of the out buffer.
 *
 * Return:
 *   NULL if no name was found.  A pointer to the passed buffer otherwise.
 */
const char *skycultures_get_label(const char *main_id, char *out, int out_size);


/*
 * Function: skycultures_get_designations
 * Get the sorted and translated list of designations for a sky object including
 * cultural names.
 *
 * Parameters:
 *   obj    - A sky object.
 *   user   - User data passed to the callback.
 *   f      - A callback function called once per designation.
 */
void skycultures_get_designations(const obj_t *obj, void *user,
              void (*f)(const obj_t *obj, void *user, const char *dsgn));

/*
 * Function: skycultures_fallback_to_international_names
 * Return whether a sky culture includes the international sky objects names as
 * as fallback when no common names is explicitly specified for a given object.
 *
 * Return:
 *   True or false.
 */
bool skycultures_fallback_to_international_names();

/*
 * Function: skycultures_md_2_html
 * Convert the passed string format from markdown to HTML.
 *
 * Parameters:
 *   md     - The markdown content to convert.
 *
 * Return:
 *   A newly allocated string. The caller is responsible for freeing it.
 */
char *skycultures_md_2_html(const char *md);

/*
 * Function: core_add_task
 * Add a function that will be executed at each frame.
 *
 * The runs as along at it returns zero.
 */
void core_add_task(int (*fun)(task_t *task, double dt), void *user);
