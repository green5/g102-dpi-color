#include "hidapi/include/hidapi.h"

string hex(const string &a) {
  string ret;
  if((a.size()%2)!=0) plog("%ld[%s]",a.size(),a.c_str()),exit(3);
  for(size_t i=0;i<a.size();i+=2) {
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

static string dump1(const void *t, size_t size) {
  string ret;
  const unsigned char *s = (const unsigned char*)t;
  for(unsigned i=0;i<size;i++)
  {
    unsigned c = s[i];
    ret += format("[%02x]",c);
  }
  return ret;
}

static hid_device *dev;
static bool done = false;
static std::thread *thr;

void setcolor(const string &c) {
  string cc = c;
  if(cc=="r") cc = "ff0000";
  if(cc=="g") cc = "00ff00";
  if(cc=="b") cc = "0000ff";
  cc = "11ff0e3a0001" + cc + "0200000000000000000000";
  cc = hex(cc);
  //plog("%s",dump1(cc.c_str(),cc.size()).c_str());
  if(dev) {
    hid_write(dev,(const unsigned char*)cc.c_str(),cc.size());
    hid_write(dev,(const unsigned char*)cc.c_str(),cc.size());
  }
}

void setdpi(unsigned dpi);

static void read_thread() {
  unsigned char t[20];
  size_t iread = 0;
  while(!done) {
    memset(t,0,sizeof(t));
    int n = hid_read(dev,t,sizeof(t)); 
    int dpi = -1;
    if(n==20 && t[0]==0x11 && t[1]==0xff && t[2]==0x0f && t[3]==0x10) dpi = t[4];
    //plog("%ld: [%d] %s dpi %d",++iread,n,dump1(t,n).c_str(),dpi);
    if(dpi>=0) setdpi(dpi);
  }
}

bool hidinit() {
  hid_init();
  char name[128] = {0};
  struct hid_device_info *devs = hid_enumerate(0x46d,0xc084);
  for(struct hid_device_info *cur_dev = devs; cur_dev;) {
    //plog("id %04hx:%04hx path %s", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path);
    //plog("serial_number %ls manufacturer %ls product %ls", cur_dev->serial_number, cur_dev->manufacturer_string, cur_dev->product_string);
    strcpy(name,cur_dev->path); // last interface 
    cur_dev = cur_dev->next;
  }
  hid_free_enumeration(devs);
  if(!name[0]) return false;
  dev = hid_open_path(name);
  if(!dev) return plog("open %s",name),false;
  thr = new std::thread(read_thread);
  auto b = hex("10ff0e8a000000");
  hid_write(dev,(const unsigned char*)b.c_str(),b.size()); 
  return true;
}

void hidexit() {
  done = true;
  hid_close(dev);
  if(thr) thr->join();
}
