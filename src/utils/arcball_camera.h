#ifndef ARCBALL_CAMERA_H
#define ARCBALL_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    // Flags for tweaking the view matrix
#define ARCBALL_CAMERA_LEFT_HANDED_BIT 1

    // * eye:
    //     * Current eye position. Will be updated to new eye position.
    // * target:
    //     * Current look target position. Will be updated to new position.
    // * up:
    //     * Camera's "up" direction. Will be updated to new up vector.
    // * view (optional):
    //     * The matrix that will be updated with the new view transform. Previous contents don't matter.
    // * delta_time_seconds:
    //     * Amount of seconds passed since last update.
    // * zoom_per_tick:
    //     * How much the camera should zoom with every scroll wheel tick.
    // * pan_speed:
    //     * How fast the camera should pan when holding middle click.
    // * rotation_multiplier:
    //     * For amplifying the rotation speed. 1.0 means 1-1 mapping between arcball rotation and camera rotation.
    // * screen_width/screen_height:
    //     * Dimensions of the screen the camera is being used in (the window size).
    // * x0, x1:
    //     * Previous and current x coordinate of the mouse, respectively.
    // * y0, y1:
    //     * Previous and current y coordinate of the mouse, respectively.
    // * midclick_held:
    //     * Whether the middle click button is currently held or not.
    // * rclick_held:
    //     * Whether the right click button is currently held or not.
    // * delta_scroll_ticks:
    //     * How many scroll wheel ticks passed since the last update (signed number)
    // * flags:
    //     * For producing a different view matrix depending on your conventions.
    void arcball_camera_update(
        float eye[3],
        float target[3],
        float up[3],
        float view[16],
        float delta_time_seconds,
        float zoom_per_tick,
        float pan_speed,
        float rotation_multiplier,
        int screen_width, int screen_height,
        int x0, int x1,
        int y0, int y1,
        int midclick_held,
        int rclick_held,
        int delta_scroll_ticks,
        unsigned int flags);

    // Utility for producing a look-to matrix without having to update a camera.
    void arcball_camera_look_to(
        const float eye[3],
        const float look[3],
        const float up[3],
        float view[16],
        unsigned int flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ARCBALL_CAMERA_H
