#ifndef XWAYLAND_SELECTION_H
#define XWAYLAND_SELECTION_H

#include <xcb/xfixes.h>

#define INCR_CHUNK_SIZE (64 * 1024)

#define XDND_VERSION 5

struct wlr_primary_selection_source;

struct wlr_xwm_selection;

struct wlr_xwm_selection_transfer {
	struct wlr_xwm_selection *selection;

	bool incr;
	bool flush_property_on_delete;
	bool property_set;
	struct wl_array source_data;
	int source_fd;
	struct wl_event_source *source;

	// when sending to x11
	xcb_selection_request_event_t request;
	struct wl_list outgoing_link;

	// when receiving from x11
	int property_start;
	xcb_get_property_reply_t *property_reply;
};

struct wlr_xwm_selection {
	struct wlr_xwm *xwm;
	xcb_atom_t atom;
	xcb_window_t window;
	xcb_window_t owner;
	xcb_timestamp_t timestamp;

	struct wlr_xwm_selection_transfer incoming;
	struct wl_list outgoing;
};

void xwm_selection_transfer_remove_source(
	struct wlr_xwm_selection_transfer *transfer);
void xwm_selection_transfer_close_source_fd(
	struct wlr_xwm_selection_transfer *transfer);
void xwm_selection_transfer_destroy_property_reply(
	struct wlr_xwm_selection_transfer *transfer);

struct wlr_xwm_selection *xwm_get_selection(struct wlr_xwm *xwm,
	xcb_atom_t selection_atom);

void xwm_send_incr_chunk(struct wlr_xwm_selection_transfer *transfer);
void xwm_handle_selection_request(struct wlr_xwm *xwm,
	xcb_selection_request_event_t *req);

void xwm_get_incr_chunk(struct wlr_xwm_selection_transfer *transfer);
void xwm_handle_selection_notify(struct wlr_xwm *xwm,
	xcb_selection_notify_event_t *event);
int xwm_handle_xfixes_selection_notify(struct wlr_xwm *xwm,
	xcb_xfixes_selection_notify_event_t *event);
bool data_source_is_xwayland(struct wlr_data_source *wlr_source);
bool primary_selection_source_is_xwayland(
	struct wlr_primary_selection_source *wlr_source);

void xwm_seat_handle_start_drag(struct wlr_xwm *xwm, struct wlr_drag *drag);

void xwm_selection_init(struct wlr_xwm *xwm);
void xwm_selection_finish(struct wlr_xwm *xwm);

enum mime_map_method {
	MAP_MIME_TO_ATOM,
	MAP_ATOM_TO_MIME,
	MAP_MIME_TO_PSEUDOMIME,
};

typedef union mime_map_arg {
	struct wlr_xwm *xwm;
	char *mime;
	const char *pseudo_mime;
	xcb_atom_t atom;
} mime_map_arg_t;

/**
 * Content type and X11 targets (atoms+verbatim) for selection data conversion.
 *
 * Respective inout members set correspondingly only on success (0 returned).
 *
 * Due to heterogenous nature of all the mappings required, we settle with
 * two operands, in and inout, arranged as unions, for which following members
 * are utilized depending on method as prescribed:
 *
 *                        |      in      | inout [in]  |  inout [out]
 * -----------------------+--------------+-------------+---------------
 * MAP_MIME_TO_ATOM       |    .mime     |    .xwm     |   .atom
 * -----------------------+--------------+-------------+---------------
 * MAP_ATOM_TO_MIME       |    .atom     |    .xwm     |   .mime
 * -----------------------+--------------+-------------+---------------
 * MAP_MIME_TO_PSEUDOMIME |    .mime     |     N/A     |   .pseudomime
 * -----------------------+--------------+-------------+---------------
 *
 * Apparently, there's seemingly a hazard with inout being used in two
 * roles, but that's not harmful as it is written to only when former
 * (input) meaning is intrinsically no longer needed.
 *
 * Returns 0 when internal lookup succeed, 1 when secondary (external)
 * lookup was successfully used, -1 otherwise.
 */
int xwm_mime_map(enum mime_map_method method, const mime_map_arg_t in,
	mime_map_arg_t *inout);

#endif
