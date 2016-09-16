/*
 * koruza-driver - KORUZA driver
 *
 * Copyright (C) 2016 Jernej Kos <jernej@kos.mx>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ubus.h"
#include "koruza.h"

#include <libubox/blobmsg.h>

// Ubus reply buffer.
static struct blob_buf reply_buf;

// Ubus attributes.
enum {
  KORUZA_MOTOR_X,
  KORUZA_MOTOR_Y,
  KORUZA_MOTOR_Z,
  __KORUZA_MOTOR_MAX,
};

static const struct blobmsg_policy koruza_motor_policy[__KORUZA_MOTOR_MAX] = {
  [KORUZA_MOTOR_X] = { .name = "x", .type = BLOBMSG_TYPE_INT32 },
  [KORUZA_MOTOR_Y] = { .name = "y", .type = BLOBMSG_TYPE_INT32 },
  [KORUZA_MOTOR_Z] = { .name = "z", .type = BLOBMSG_TYPE_INT32 },
};

static int ubus_move_motor(struct ubus_context *ctx, struct ubus_object *obj,
                           struct ubus_request_data *req, const char *method,
                           struct blob_attr *msg)
{
  struct blob_attr *tb[__KORUZA_MOTOR_MAX];

  blobmsg_parse(koruza_motor_policy, __KORUZA_MOTOR_MAX, tb, blob_data(msg), blob_len(msg));

  if (!tb[KORUZA_MOTOR_X] || !tb[KORUZA_MOTOR_Y] || !tb[KORUZA_MOTOR_Z]) {
    return UBUS_STATUS_INVALID_ARGUMENT;
  }

  int result = koruza_move_motor(
    (int32_t) blobmsg_get_u32(tb[KORUZA_MOTOR_X]),
    (int32_t) blobmsg_get_u32(tb[KORUZA_MOTOR_Y]),
    (int32_t) blobmsg_get_u32(tb[KORUZA_MOTOR_Z])
  );

  return result < 0 ? UBUS_STATUS_UNKNOWN_ERROR : UBUS_STATUS_OK;
}

static int ubus_get_status(struct ubus_context *ctx, struct ubus_object *obj,
                           struct ubus_request_data *req, const char *method,
                           struct blob_attr *msg)
{
  const struct koruza_status *status = koruza_get_status();
  void *c;

  blob_buf_init(&reply_buf, 0);
  blobmsg_add_u8(&reply_buf, "connected", status->connected);

  c = blobmsg_open_table(&reply_buf, "motors");
  blobmsg_add_u32(&reply_buf, "x", status->motors.x);
  blobmsg_add_u32(&reply_buf, "y", status->motors.y);
  blobmsg_add_u32(&reply_buf, "z", status->motors.z);
  blobmsg_close_table(&reply_buf, c);

  c = blobmsg_open_table(&reply_buf, "camera_calibration");
  blobmsg_add_u32(&reply_buf, "width", status->camera_calibration.width);
  blobmsg_add_u32(&reply_buf, "height", status->camera_calibration.height);
  blobmsg_add_u32(&reply_buf, "offset_x", status->camera_calibration.offset_x);
  blobmsg_add_u32(&reply_buf, "offset_y", status->camera_calibration.offset_y);
  blobmsg_add_u32(&reply_buf, "distance", status->camera_calibration.distance);
  blobmsg_close_table(&reply_buf, c);

  ubus_send_reply(ctx, req, reply_buf.head);

  return UBUS_STATUS_OK;
}

static const struct ubus_method koruza_methods[] = {
  UBUS_METHOD("move_motor", ubus_move_motor, koruza_motor_policy),
  UBUS_METHOD_NOARG("get_status", ubus_get_status)
};

static struct ubus_object_type koruza_type =
  UBUS_OBJECT_TYPE("koruza", koruza_methods);

static struct ubus_object koruza_object = {
  .name = "koruza",
  .type = &koruza_type,
  .methods = koruza_methods,
  .n_methods = ARRAY_SIZE(koruza_methods),
};

int ubus_init(struct ubus_context *ubus)
{
  return ubus_add_object(ubus, &koruza_object);
}
