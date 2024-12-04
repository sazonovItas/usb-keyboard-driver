#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME "usbkbd"
#endif

#include <linux/hid.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb/input.h>

/**
 * Version information
 */
#define DRIVER_VERSION "v1.0"
#define DRIVER_NAME "usbkbd"
#define DRIVER_LICENSE "GPL"
#define DRIVER_AUTHOR "itas <itassudo@gmail.com>"

#define USB_INTERFACE_CLASS_HID 3
#define USB_INTERFACE_SUBCLASS_BOOT 1
#define USB_INTERFACE_PROTOCOL_KEYBOARD 1

#define KEYCODE_BUF_LEN 256
#define NAME_BUF_LEN 128
#define PHYS_BUF_LEN 64

#define KEY_RELEASED 0
#define KEY_PRESSED 1

static const u8 usb_kbd_keycode[KEYCODE_BUF_LEN] = {
    0,   0,   0,   0,   30,  48,  46,  32,  18,  33,  34,  35,  23,  36,  37,
    38,  50,  49,  24,  25,  16,  19,  31,  20,  22,  47,  17,  45,  21,  44,
    2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  28,  1,   14,  15,  57,
    12,  13,  26,  27,  43,  43,  39,  40,  41,  51,  52,  53,  58,  59,  60,
    61,  62,  63,  64,  65,  66,  67,  68,  87,  88,  99,  70,  119, 110, 102,
    104, 111, 107, 109, 106, 105, 108, 103, 69,  98,  55,  74,  78,  96,  79,
    80,  81,  75,  76,  77,  71,  72,  73,  82,  83,  86,  127, 116, 117, 183,
    184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 134, 138, 130, 132,
    128, 129, 131, 137, 133, 135, 136, 113, 115, 114, 0,   0,   0,   121, 0,
    89,  93,  124, 92,  94,  95,  0,   0,   0,   122, 123, 90,  91,  85,  0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   29,
    42,  56,  125, 97,  54,  100, 126, 164, 166, 165, 163, 161, 115, 114, 113,
    150, 158, 159, 128, 136, 177, 178, 176, 142, 152, 173, 140};

struct usb_kbd {
  struct input_dev *indev;
  struct usb_device *usbdev;

  char name[NAME_BUF_LEN];
  char phys[PHYS_BUF_LEN];

  struct urb *irq, *led;
  struct usb_ctrlrequest *cr;

  u8 *new, *leds;
  u8 old[8];

  dma_addr_t new_dma, leds_dma;

  spinlock_t keys_lock, leds_lock;
  bool led_urb_submitted;
  bool mode;
};

void *__memscan_s(void *start, int value, __kernel_size_t sz) {
  void *pos = memscan(start, value, sz);
  if (pos == start + sz)
    return NULL;

  return pos;
}

static void usb_kbd_irq(struct urb *urb) {
  int error;
  struct usb_kbd *kbd = urb->context;

  switch (urb->status) {
  case 0:
    break;
  case -ECONNRESET:
  case -ENOENT:
  case -ESHUTDOWN:
    return;

  default:
    goto resubmit;
  }

  for (int i = 0; i < 8; i++)
    input_report_key(kbd->indev, usb_kbd_keycode[i + 224],
                     (kbd->new[0] >> i) & 1);

  for (int i = 2; i < 8; i++) {
    if (kbd->old[i] > 3 && __memscan_s(kbd->new + 2, kbd->old[i], 6) == NULL) {
      if (usb_kbd_keycode[kbd->old[i]])
        input_report_key(kbd->indev, usb_kbd_keycode[kbd->old[i]],
                         KEY_RELEASED);
      else
        hid_info(urb->dev, "Unknown key (scancode %#x) released.\n",
                 kbd->old[i]);
    }

    if (kbd->new[i] > 3 && __memscan_s(kbd->old + 2, kbd->new[i], 6) == NULL) {
      if (usb_kbd_keycode[kbd->new[i]])
        input_report_key(kbd->indev, usb_kbd_keycode[kbd->new[i]], KEY_PRESSED);
      else
        hid_info(urb->dev, "Unknown key (scancode %#x) pressed.\n",
                 kbd->new[i]);
    }
  }

  input_sync(kbd->indev);
  memcpy(kbd->old, kbd->new, 8);

resubmit:
  error = usb_submit_urb(urb, GFP_ATOMIC);
  if (error) {
    hid_err(urb->dev, "can't resubmit intr, %s-%s/input0, status %d",
            kbd->usbdev->bus->bus_name, kbd->usbdev->devpath, error);
  }
}

static int usb_kbd_event(struct input_dev *input_dev, unsigned int type,
                         unsigned int code, int value) {
  struct usb_kbd *kbd = input_get_drvdata(input_dev);

  spin_lock_irq(&kbd->keys_lock);
  pr_alert("%s: event type: %d -> code: %d, value: %d", DRIVER_NAME, type, code,
           value);
  spin_unlock_irq(&kbd->keys_lock);

  return 0;
}
static int usb_kbd_open(struct input_dev *input_dev) {
  struct usb_kbd *kbd = input_get_drvdata(input_dev);

  /*add print*/
  pr_alert("%s: open USB keyboard device", DRIVER_NAME);
  /*add print*/

  kbd->irq->dev = kbd->usbdev;
  if (usb_submit_urb(kbd->irq, GFP_KERNEL)) {
    return -EIO;
  }

  return 0;
}

static void usb_kbd_close(struct input_dev *input_dev) {
  struct usb_kbd *kbd = input_get_drvdata(input_dev);

  /*add print*/
  pr_alert("%s: close USB keyboard device", DRIVER_NAME);
  /*add print*/

  usb_kill_urb(kbd->irq);
}

static int usb_kbd_alloc_mem(struct usb_device *dev, struct usb_kbd *kbd) {
  if (!(kbd->irq = usb_alloc_urb(0, GFP_KERNEL)))
    return -1;
  if (!(kbd->led = usb_alloc_urb(0, GFP_KERNEL)))
    return -1;
  if (!(kbd->new = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &kbd->new_dma)))
    return -1;
  // if (!(kbd->leds = usb_alloc_coherent(dev, 1, GFP_ATOMIC, &kbd->leds_dma)))
  //   return -1;
  return 0;
}
static void usb_kbd_free_mem(struct usb_device *dev, struct usb_kbd *kbd) {
  usb_free_urb(kbd->irq);
  usb_free_urb(kbd->led);
  usb_free_coherent(dev, 8, kbd->new, kbd->new_dma);
  // usb_free_coherent(dev, 1, kbd->leds, kbd->leds_dma);
}

static int usb_kbd_probe(struct usb_interface *intf,
                         const struct usb_device_id *id) {

  struct usb_device *usb_dev = interface_to_usbdev(intf);
  struct usb_host_interface *iface_desc;
  struct usb_endpoint_descriptor *endpoint;
  struct input_dev *input_dev;
  struct usb_kbd *kbd;

  int error = -ENOMEM;

  /*add print*/
  pr_alert("%s: probing USB keyboard device %s", DRIVER_NAME, usb_dev->product);
  /*add print*/

  iface_desc = intf->cur_altsetting;
  if (iface_desc->desc.bNumEndpoints > 1)
    return -ENODEV;

  endpoint = &iface_desc->endpoint[0].desc;
  if (!usb_endpoint_is_int_in(endpoint))
    return -ENODEV;

  int pipe, maxp;
  pipe = usb_rcvintpipe(usb_dev, endpoint->bEndpointAddress);
  maxp = usb_maxpacket(usb_dev, pipe);

  kbd = kzalloc(sizeof(struct usb_kbd), GFP_KERNEL);
  input_dev = input_allocate_device();
  if (!input_dev || !kbd)
    goto fail_allocate_mem;

  if (usb_kbd_alloc_mem(usb_dev, kbd))
    goto fail_allocate_kbd_mem;

  kbd->usbdev = usb_dev;
  kbd->indev = input_dev;
  spin_lock_init(&kbd->leds_lock);
  spin_lock_init(&kbd->keys_lock);

  if (usb_dev->manufacturer)
    strlcpy(kbd->name, usb_dev->manufacturer, sizeof(kbd->name));

  if (usb_dev->product) {
    if (usb_dev->manufacturer)
      strlcat(kbd->name, " ", sizeof(kbd->name));
    strlcat(kbd->name, usb_dev->product, sizeof(kbd->name));
  }

  if (!strlen(kbd->name))
    snprintf(kbd->name, sizeof(kbd->name), "USB HIDBP Keyboard %04x:%04x",
             le16_to_cpu(usb_dev->descriptor.idVendor),
             le16_to_cpu(usb_dev->descriptor.idProduct));

  usb_make_path(usb_dev, kbd->phys, sizeof(kbd->phys));
  strlcat(kbd->phys, "/input0", sizeof(kbd->phys));

  input_dev->name = kbd->name;
  input_dev->phys = kbd->phys;
  usb_to_input_id(usb_dev, &input_dev->id);
  input_dev->dev.parent = &intf->dev;

  input_set_drvdata(input_dev, kbd);

  input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_LED) | BIT_MASK(EV_REP);
  input_dev->ledbit[0] = BIT_MASK(LED_NUML) | BIT_MASK(LED_CAPSL) |
                         BIT_MASK(LED_SCROLLL) | BIT_MASK(LED_COMPOSE) |
                         BIT_MASK(LED_KANA);

  for (int i = 0; i < KEYCODE_BUF_LEN; i++) {
    set_bit(usb_kbd_keycode[i], input_dev->keybit);
  }
  clear_bit(0, input_dev->keybit);

  input_dev->event = usb_kbd_event;
  input_dev->open = usb_kbd_open;
  input_dev->close = usb_kbd_close;

  usb_fill_int_urb(kbd->irq, kbd->usbdev, pipe, kbd->new, (maxp > 8 ? 8 : maxp),
                   usb_kbd_irq, kbd, endpoint->bInterval);
  kbd->irq->transfer_dma = kbd->new_dma;
  kbd->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

  error = input_register_device(kbd->indev);
  if (error)
    goto fail_allocate_kbd_mem;

  usb_set_intfdata(intf, kbd);
  device_set_wakeup_enable(&usb_dev->dev, true);
  return 0;

fail_allocate_kbd_mem:
  usb_kbd_free_mem(kbd->usbdev, kbd);
fail_allocate_mem:
  input_free_device(input_dev);
  kfree(kbd);
  return error;
}

static void usb_kbd_disconnect(struct usb_interface *intf) {

  struct usb_kbd *kbd = usb_get_intfdata(intf);

  /*add print*/
  pr_alert("%s: USB keyboard driver is removed\n", DRIVER_NAME);
  /*add print*/

  if (kbd) {
    input_unregister_device(kbd->indev);
    usb_kbd_free_mem(interface_to_usbdev(intf), kbd);
    kfree(kbd);
  }

  return;
}

static int usb_kbd_suspend(struct usb_interface *intf, pm_message_t msg) {
  return 0;
}
static int usb_kbd_resume(struct usb_interface *intf) { return 0; }
static int usb_kbd_reset_resume(struct usb_interface *intf) { return 0; }

static struct usb_device_id usb_kbd_table[] = {
    {USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
                        USB_INTERFACE_PROTOCOL_KEYBOARD)},
    {}};
MODULE_DEVICE_TABLE(usb, usb_kbd_table);

static struct usb_driver usb_kbd_driver = {
    .name = DRIVER_NAME,
    .probe = usb_kbd_probe,
    .disconnect = usb_kbd_disconnect,
    .suspend = usb_kbd_suspend,
    .resume = usb_kbd_resume,
    .reset_resume = usb_kbd_reset_resume,
    .id_table = usb_kbd_table,
};

module_usb_driver(usb_kbd_driver);

MODULE_LICENSE(DRIVER_LICENSE);
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR(DRIVER_AUTHOR);
