#include "std.h"
#include "str.h"
#include <libusb-1.0/libusb.h>

using namespace std;

struct Exception : std::exception {
  string a;
  Exception(const string &a_):a(a_) {}
  virtual const char* what() const noexcept { return a.c_str(); }
};

string hex(const char *a) {
  string ret;
  for(size_t i=0;a[i];i+=2) {
    unsigned c1 = isdigit(a[i]) ? a[i]-'0' : (10+a[i]-(islower(a[i])?'a':'A'));
    unsigned c2 = isdigit(a[i+1]) ? a[i+1]-'0' : (10+a[i+1]-(islower(a[i+1])?'a':'A'));
    ret += (c1<<4)|c2;
  }
  return ret;
}

string unhex(const string &a) {
  string ret;
  for(int c:a) {
    c &= 255;
    int c1 = (c>>4)&255;
    int c2 = c&15;
    c1 += c1>9?('a'-10):'0';
    c2 += c2>9?('a'-10):'0';
    ret += c1;
    ret += c2;
  }
  return ret;
}

void send(struct libusb_device_handle *devh,const string &data,unsigned wValue) {
  auto n = data.size();
  if(!n) return;
  if((n%2)!=0) { perr("BadString:%s[%ld]",data.c_str(),n); return; }
  string b = hex(data.c_str());
  int ok = libusb_control_transfer(devh, 0x21, 0x09, wValue, 0x01, (unsigned char*)b.c_str(), b.size(), 1000);
  plog("send[%s] ok %d(%s) size %ld", data.c_str(), ok, libusb_error_name(ok), b.size());
}

void setcolor(struct libusb_device_handle *devh, const string &c) {
  string cc = c;
  if(cc=="r") cc = "ff0000";
  if(cc=="g") cc = "00ff00";
  if(cc=="b") cc = "0000ff";
  send(devh,"11ff0e3a0001"+cc+"0200000000000000000000",0x0211);
  send(devh,"11ff0e3a0001"+cc+"0200000000000000000000",0x0211); 
}

struct g102 {
  uint16_t x11ff;
  uint8_t src;
  uint8_t x10;
  uint8_t data;        
} __attribute__((packed));

int set_dpi = -1;

static void read_callback(struct libusb_transfer *transfer)
{
  string b((char*)transfer->buffer,transfer->length);
  struct g102 *g = (struct g102*)b.c_str();
  int dpi = -1;
  if(transfer->status==0 && g->src==0x0f) dpi = g->data; // push
  //if(transfer->status==2 && g->src==0x0f) dpi = g->data; // bulk
  plog("read %d[%d] status %d end 0x%x data %s dpi %d",transfer->length,transfer->actual_length,transfer->status,transfer->endpoint,STR_H_::dump1(b).c_str(),dpi);
  if(dpi>=0) set_dpi = dpi;
  int ok = libusb_submit_transfer(transfer);
  if(ok) perr("%d",ok);
}

static bool done = false;

static void read_thread(struct libusb_device_handle *dev,libusb_context *usb_context, int interface_number, int endpoint_number)
{
  // https://vovkos.github.io/doxyrest/samples/libusb/group_libusb_asyncio.html
  string buf(20,0);
  auto transfer = libusb_alloc_transfer(0);
  libusb_fill_interrupt_transfer(transfer,dev,endpoint_number,(unsigned char*)buf.c_str(),buf.size(),read_callback,dev,5000);
  libusb_submit_transfer(transfer);
  while(!done) {
    int ok = libusb_handle_events(usb_context);
    if(ok==LIBUSB_SUCCESS) {
      if(set_dpi>=0) {
        string c = "r";
        if(set_dpi>=1) c = "g";
        if(set_dpi>=2) c = "b";
        if(set_dpi>=3) c = "ffffff";
        set_dpi = -1;
        setcolor(transfer->dev_handle,c);
      }
      continue;
    }
    plog("libusb_handle_events = %d(%s)",ok,libusb_error_name(ok));
    if(ok==LIBUSB_ERROR_BUSY) continue;
    if(ok==LIBUSB_ERROR_TIMEOUT) continue;
    if(ok==LIBUSB_ERROR_OVERFLOW) continue;
    if(ok==LIBUSB_ERROR_INTERRUPTED) continue;
    break;
  }
  libusb_cancel_transfer(transfer);
  libusb_free_transfer(transfer);
  //int cancelled = 0; while (!cancelled) libusb_handle_events_completed(usb_context, &cancelled);
  plog("thread done");
}

#define ERR(ok) perr("libusb=%d(%s)",ok,libusb_error_name(ok))

int main(int ac,char *av[]) {
  int interface_number = 1;
  int endpoint_number = LIBUSB_ENDPOINT_IN|(1+interface_number);
  libusb_context *ctx = NULL;
  int ok = libusb_init(&ctx);
  if(ok) return perr("libusb_init=%d",ok);
  struct libusb_device_handle *devh = libusb_open_device_with_vid_pid(ctx,0x046d,0xc084);
  if(!devh) return plog("Device 0x046d:0xc084 not found");

  struct libusb_device_descriptor desc;
  struct libusb_config_descriptor *conf_desc = NULL;
  libusb_device *dev = libusb_get_device(devh);
  ok = libusb_get_device_descriptor(dev, &desc);
  if(ok) ERR(ok);
  ok = libusb_get_active_config_descriptor(dev, &conf_desc);
  if(ok) ERR(ok);
  if(conf_desc) for (int j = 0; j < conf_desc->bNumInterfaces; j++) {
    const struct libusb_interface *intf = &conf_desc->interface[j];
    for(int k = 0; k < intf->num_altsetting; k++) {
      const struct libusb_interface_descriptor *intf_desc = &intf->altsetting[k];
      for(int e = 0; e<intf_desc->bNumEndpoints; e++) {
        const libusb_endpoint_descriptor *endp_desc = &intf_desc->endpoint[e];
        plog("InterfaceNumber 0x%02x EndpointAddress 0x%02x", intf_desc->bInterfaceNumber, endp_desc->bEndpointAddress);
        if(intf_desc->bInterfaceNumber==interface_number) endpoint_number = endp_desc->bEndpointAddress;
      }
    }
  }

  ok = libusb_kernel_driver_active(devh,interface_number);
  if(ok) {
    ok = libusb_detach_kernel_driver(devh,interface_number);
    if(ok) perr("libusb_detach_kernel_driver=%d [%s]",ok,libusb_error_name(ok));
  }
  ok = libusb_claim_interface(devh, interface_number);
  if(ok) perr("libusb_claim_interface=%d [%s]",ok,libusb_error_name(ok));

  //libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

  std::thread thr(read_thread, devh, ctx, interface_number, endpoint_number);

  if(ac>1) {
    send(devh,"10ff0e8a000000",0x0210);
    setcolor(devh,av[1]);
  }
  //done = true;
  thr.join();
  libusb_release_interface(devh, interface_number);
  libusb_close(devh);
  libusb_exit(ctx);
  return 0;
}
