/* Spa V4l2 Source
 *
 * Copyright © 2018 Wim Taymans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/videodev2.h>

#include <spa/support/log.h>
#include <spa/support/loop.h>
#include <spa/monitor/device.h>
#include <spa/debug/pod.h>

#include "v4l2.h"

#define NAME "v4l2-device"

static const char default_device[] = "/dev/video0";

extern const struct spa_handle_factory spa_v4l2_source_factory;

struct props {
	char device[64];
	char device_name[128];
	int device_fd;
};

static void reset_props(struct props *props)
{
	strncpy(props->device, default_device, 64);
}

struct impl {
	struct spa_handle handle;
	struct spa_device device;

	struct spa_log *log;
	struct spa_loop *main_loop;

	struct props props;

	const struct spa_device_callbacks *callbacks;
	void *callbacks_data;

	struct spa_v4l2_device dev;
};

static int impl_device_set_callbacks(struct spa_device *device,
				   const struct spa_device_callbacks *callbacks,
				   void *data)
{
	struct impl *this;
	struct spa_dict_item items[1];

	spa_return_val_if_fail(device != NULL, -EINVAL);

	this = SPA_CONTAINER_OF(device, struct impl, device);

	this->callbacks = callbacks;
	this->callbacks_data = data;

	if (callbacks) {
		if (spa_v4l2_is_capture(&this->dev)) {
			items[0] = SPA_DICT_ITEM_INIT("device.path", this->props.device);

			if (callbacks->add)
				callbacks->add(data, 0,
					&spa_v4l2_source_factory,
					SPA_TYPE_INTERFACE_Node,
					&SPA_DICT_INIT(items, 1));
		}


	}

	return 0;
}

static const struct spa_dict_item info_items[] = {
	{ "media.class", "Video/Device" },
};

static const struct spa_dict info = {
	info_items,
	SPA_N_ELEMENTS(info_items)
};

static const struct spa_device impl_device = {
	SPA_VERSION_DEVICE,
	&info,
	impl_device_set_callbacks,
};

static int impl_get_interface(struct spa_handle *handle, uint32_t type, void **interface)
{
	struct impl *this;

	spa_return_val_if_fail(handle != NULL, -EINVAL);
	spa_return_val_if_fail(interface != NULL, -EINVAL);

	this = (struct impl *) handle;

	if (type == SPA_TYPE_INTERFACE_Device)
		*interface = &this->device;
	else
		return -ENOENT;

	return 0;
}

static int impl_clear(struct spa_handle *handle)
{
	return 0;
}

static size_t
impl_get_size(const struct spa_handle_factory *factory,
	      const struct spa_dict *params)
{
	return sizeof(struct impl);
}

static int
impl_init(const struct spa_handle_factory *factory,
	  struct spa_handle *handle,
	  const struct spa_dict *info,
	  const struct spa_support *support,
	  uint32_t n_support)
{
	struct impl *this;
	uint32_t i;
	const char *str;
	int res;

	spa_return_val_if_fail(factory != NULL, -EINVAL);
	spa_return_val_if_fail(handle != NULL, -EINVAL);

	handle->get_interface = impl_get_interface;
	handle->clear = impl_clear, this = (struct impl *) handle;

	for (i = 0; i < n_support; i++) {
		if (support[i].type == SPA_TYPE_INTERFACE_Log)
			this->log = support[i].data;
		else if (support[i].type == SPA_TYPE_INTERFACE_MainLoop)
			this->main_loop = support[i].data;
	}
	if (this->main_loop == NULL) {
		spa_log_error(this->log, "a main_loop is needed");
		return -EINVAL;
	}

	this->device = impl_device;
	this->dev.log = this->log;
	this->dev.fd = -1;

	reset_props(&this->props);

	if (info && (str = spa_dict_lookup(info, "device.path"))) {
		strncpy(this->props.device, str, 63);
		if ((res = spa_v4l2_open(&this->dev, this->props.device)) < 0)
			return res;
		spa_v4l2_close(&this->dev);
	}

	return 0;
}

static const struct spa_interface_info impl_interfaces[] = {
	{SPA_TYPE_INTERFACE_Device,},
};

static int impl_enum_interface_info(const struct spa_handle_factory *factory,
				    const struct spa_interface_info **info,
				    uint32_t *index)
{
	spa_return_val_if_fail(factory != NULL, -EINVAL);
	spa_return_val_if_fail(info != NULL, -EINVAL);
	spa_return_val_if_fail(index != NULL, -EINVAL);

	if (*index >= SPA_N_ELEMENTS(impl_interfaces))
		return 0;

	*info = &impl_interfaces[(*index)++];

	return 1;
}

const struct spa_handle_factory spa_v4l2_device_factory = {
	SPA_VERSION_HANDLE_FACTORY,
	NAME,
	NULL,
	impl_get_size,
	impl_init,
	impl_enum_interface_info,
};
