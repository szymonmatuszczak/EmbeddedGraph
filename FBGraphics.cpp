using namespace std;

const unsigned char SUCCESS = 0;
const unsigned char CANNOT_OPEN_FRAMEBUFFER_DEVICE = 1;
const unsigned char CANNOT_READ_FIXED_INFORMATION = 2;
const unsigned char CANNOT_READ_VARIABLE_INFORMATION = 3;
const unsigned char CANNOT_MAP_FRAMEBUFFER_DEVICE_TO_MEMORY = 4;
const unsigned char UNSUPPORTED_FRAMEBUFFER_DEVICE_BPP = 5;

class FBGraphics
{
	private:
		int FBhandler = 0;
		struct fb_fix_screeninfo FBfixedInfo;
		struct fb_var_screeninfo FBvariableInfo;
		long int FBsize = 0; //bytes
		unsigned char *FBmemoryStart = 0;
		string FBdeviceName;
		cairo_format_t bpp;
		cairo_format_t getBPPforCairo()
		{
			if(FBvariableInfo.bits_per_pixel == 16) return CAIRO_FORMAT_RGB16_565;
			else if(FBvariableInfo.bits_per_pixel == 24) return CAIRO_FORMAT_RGB24;
			else if(FBvariableInfo.bits_per_pixel == 30) return CAIRO_FORMAT_RGB30;
			else if(FBvariableInfo.bits_per_pixel == 32) return CAIRO_FORMAT_ARGB32;
			else return CAIRO_FORMAT_INVALID;
		}
		void die(const unsigned char error)
		{
			cout << "Cannot initialize framebuffer device, error code: " << error << endl;
			exit(1);
		}
		bool checkAssetBPP(asset checkable)
		{
			if(checkable.bytes_per_pixel == 2 && FBvariableInfo.bits_per_pixel == 16) return true;
			if(checkable.bytes_per_pixel == 3 && FBvariableInfo.bits_per_pixel == 24) return true;
			if(checkable.bytes_per_pixel == 4 && FBvariableInfo.bits_per_pixel == 32) return true;
			return false;
		}
	public:
		thread watek1;
		cairo_t	*cairo;
		cairo_surface_t *surface;
		FBGraphics(const string FBdeviceNameL)
		{
			FBdeviceName = FBdeviceNameL;
			FBhandler = open(FBdeviceName.c_str(), O_RDWR);
			if(FBhandler == -1) die(CANNOT_OPEN_FRAMEBUFFER_DEVICE);
			if(ioctl(FBhandler, FBIOGET_FSCREENINFO, &FBfixedInfo) == -1) die(CANNOT_READ_FIXED_INFORMATION);
			if(ioctl(FBhandler, FBIOGET_VSCREENINFO, &FBvariableInfo) == -1) die(CANNOT_READ_VARIABLE_INFORMATION);
			FBsize = (FBvariableInfo.xres*FBvariableInfo.yres*FBvariableInfo.bits_per_pixel)/8;
			FBmemoryStart = (unsigned char*)mmap(0, FBsize, PROT_READ|PROT_WRITE, MAP_SHARED, FBhandler, 0);
			//if((int)FBmemoryStart == -1) die(CANNOT_MAP_FRAMEBUFFER_DEVICE_TO_MEMORY); //na niektórych kompilatorach/platformach stwarza problemy przy kompilacji
			bpp = getBPPforCairo();
			if(bpp == CAIRO_FORMAT_INVALID) die(UNSUPPORTED_FRAMEBUFFER_DEVICE_BPP);
			surface = cairo_image_surface_create_for_data(FBmemoryStart, bpp, FBvariableInfo.xres, FBvariableInfo.yres, cairo_format_stride_for_width(bpp, FBvariableInfo.xres));
			cairo = cairo_create(surface);
		}
		void destroy()
		{
			cairo_destroy(cairo);
			cairo_surface_destroy(surface);
			munmap(FBmemoryStart, FBsize);
			close(FBhandler);
		}
		void renderAsset(const unsigned int x, const unsigned int y, const asset checkable)
		{
			if(checkAssetBPP(checkable))
			{
				if(x < FBvariableInfo.xres && y < FBvariableInfo.yres)
				{
					const unsigned int stride = cairo_format_stride_for_width(bpp, FBvariableInfo.xres);
					const unsigned int realWidth = FBvariableInfo.xres-x;
					const unsigned int realHeight = checkable.height;
					unsigned char* sourceStart = checkable.data_start;
					unsigned char* destinationStart = cairo_image_surface_get_data(surface)+(stride*y);
					cairo_surface_flush(surface);
					for(unsigned int i=0;i<realHeight;i++)
					{
						destinationStart += x*checkable.bytes_per_pixel;
						for(unsigned int j=0;j<realWidth*checkable.bytes_per_pixel;j++)
						{
							*destinationStart = *sourceStart;
							destinationStart++;
							sourceStart++;
						}
						sourceStart += x*checkable.bytes_per_pixel;
					}
					cairo_surface_mark_dirty(surface);
				}
				else
				{
					cout << "Invalid coordinates!" << endl;
					exit(1);
				}
			}
			else
			{
				cout << "Invalid asset format!" << endl;
				exit(1);
			}
		}
		void RenderText(const string text, const double xP=0, const double yP=0, const string fontName="Roboto-Regular", const unsigned int fontSize=11, const unsigned char r=0, const unsigned char g=0, const unsigned char b=0, const float spacing=0)
		{
			double x = xP;
			double y = yP;
			if(spacing > 0)
			{
				for(string::const_iterator it=text.begin();it!=text.end();it++)
				{
					cairo_select_font_face(cairo, fontName.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
					cairo_set_font_size(cairo, fontSize);
					cairo_set_source_rgb(cairo, r/255.0, g/255.0, b/255.0);
					cairo_move_to(cairo, x, y);
					stringstream sss;
					sss << *it;
					cairo_show_text(cairo, sss.str().c_str());
					cairo_get_current_point(cairo, &x, &y);
					x += spacing;
				}
			}
			else
			{
				cairo_select_font_face(cairo, fontName.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
				cairo_set_font_size(cairo, fontSize);
				cairo_set_source_rgb(cairo, r/255.0, g/255.0, b/255.0);
				cairo_move_to(cairo, x, y);
				cairo_show_text(cairo, text.c_str());
			}
		}
};