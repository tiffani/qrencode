#include <ruby.h>
#include <png.h>
#include <stdio.h>
#include <math.h>
#include <qrencode.h>

#define RGB_BYTES_PER_PIXEL 3
#define PNG_SCALE_FACTOR 10

typedef struct {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
} pixel_t;

VALUE rb_qrCodeClass;

static VALUE rb_cQRcode_init(VALUE self);
static VALUE rb_cQRcode_alloc(VALUE klass);
static VALUE rb_cQRcode_data_get(VALUE self);
static VALUE rb_cQRcode_data_set(VALUE self, VALUE str_to_encode);
static VALUE rb_cQRcode_data_encode(VALUE self);
static VALUE rb_cQRcode_write_png(VALUE self, VALUE file_name_str);
static void qr_code_free(void *p);

void Init_qrencoder()
{
	rb_qrCodeClass = rb_define_class("QRcode", rb_cObject);
	rb_define_alloc_func(rb_qrCodeClass, rb_cQRcode_alloc);
	rb_define_method(rb_qrCodeClass, "initialize", rb_cQRcode_init, 0);
	rb_define_method(rb_qrCodeClass, "data",  rb_cQRcode_data_get, 0);
	rb_define_method(rb_qrCodeClass, "data=", rb_cQRcode_data_set, 1);
	rb_define_method(rb_qrCodeClass, "encode", rb_cQRcode_data_encode, 0);
	rb_define_method(rb_qrCodeClass, "write_png", rb_cQRcode_write_png, 1);
	//rb_define_method(rb_qrCodeClass, "string_to_encode", rb_cQRcode_data_encode, )
}

static void qr_code_free(void *p)
{
	QRcode_free(p);
}

static VALUE rb_cQRcode_alloc(VALUE klass)
{
	QRcode *qr_code = ALLOC_N(QRcode, 1);
	return Data_Wrap_Struct(klass, 0, qr_code_free, qr_code);
}

static VALUE rb_cQRcode_init(VALUE self)
{
	QRcode *qr_code;
	Data_Get_Struct(self, QRcode, qr_code);
	return self;
}

static VALUE rb_cQRcode_data_set(VALUE self, VALUE str_to_encode)
{
	QRcode *qr_code;
	Data_Get_Struct(self, QRcode, qr_code);
	
	if(qr_code)
	{
		rb_iv_set(self, "@data", str_to_encode);
		return str_to_encode;
	}
	
	return Qnil;
}

static VALUE rb_cQRcode_data_encode(VALUE self)
{
	QRcode *qr_code, *encoded_qr;
	Data_Get_Struct(self, QRcode, qr_code);
	
	if(qr_code)
	{
		VALUE r_str = rb_iv_get(self, "@data");
		char *str = StringValuePtr(r_str);
		encoded_qr = malloc(sizeof(struct QRcode*));
		encoded_qr = QRcode_encodeString8bit(str, 1, QR_ECLEVEL_H);
		
		if(encoded_qr)
		{
			qr_code->width = encoded_qr->width;
			qr_code->data = malloc(strlen(encoded_qr->data) + 1);
			qr_code->version = encoded_qr->version;
			strcpy(qr_code->data, encoded_qr->data);
			QRcode_free(encoded_qr);
		}
	}
	
	return Qnil;
}

static VALUE rb_cQRcode_data_get(VALUE self)
{
	return rb_iv_get(self, "@data");
}

static VALUE rb_cQRcode_write_png(VALUE self, VALUE file_name_str)
{
	char *file_name = StringValuePtr(file_name_str);
	
	if(file_name)
	{
		FILE *fp = fopen(file_name, "wb");
		
		if(fp)
		{
			QRcode *qr_code;
			Data_Get_Struct(self, QRcode, qr_code);
			png_structp png_write_p = NULL;
			png_infop png_write_info_p = NULL;
			png_structp png_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			png_text png_title;
			png_byte **row_pointers = NULL;
			int x, y;
			
			if(qr_code)
			{
				if(!png_p)
					return Qnil;
			
				png_write_info_p = png_create_info_struct(png_p);
			
				if(png_write_info_p == NULL) {
					fclose(fp);
					fprintf(stderr, "Failed to initialize PNG write.\n");
					exit(EXIT_FAILURE);
				}
			
				if(setjmp(png_jmpbuf(png_p)))
				{
					png_destroy_write_struct(&png_p, &png_write_info_p);
					fclose(fp);
					fprintf(stderr, "Failed to write PNG image.\n");
					exit(EXIT_FAILURE);
				}
			
				png_init_io(png_p, fp);
				
				int w = (qr_code->width + 8) * PNG_SCALE_FACTOR;
				int h = w;
				
				png_set_IHDR(png_p, png_write_info_p, w, h, 8, PNG_COLOR_TYPE_RGB, 
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
			
				png_color_8 sig_bit;
				sig_bit.red = 8;
				sig_bit.green = 8;
				sig_bit.blue = 8;
				
				png_set_sBIT(png_p, png_write_info_p, &sig_bit);
				
				png_write_info(png_p, png_write_info_p);
				
				row_pointers = png_malloc(png_p, w * sizeof(png_byte *));
				
				int original_col_idx = 0, original_row_idx = 0;
				
				/* Write the top frame. */
				png_byte *row = png_malloc(png_p, sizeof(png_byte) * w * RGB_BYTES_PER_PIXEL);
				pixel_t *px = (pixel_t*)malloc(1 * sizeof(pixel_t));
				px->red = 0x00;
				px->green = 0x99;
				px->blue = 0xFF;
				
				pixel_t *bpx = (pixel_t*)malloc(1 * sizeof(pixel_t));
				bpx->red = 0xFF;
				bpx->blue = 0xFF;
				bpx->green = 0xFF;
								
				for(y = 0; y < 41; y++)
				{
					row = png_malloc(png_p, sizeof(png_byte) * w * RGB_BYTES_PER_PIXEL);
					row_pointers[y] = row;
					for(x = 0; x < w; x++)
					{
						*row++ = bpx->red;
						*row++ = bpx->green;
						*row++ = bpx->blue;
					}
				}
								
				for(y = 41; y < h; y++)
				{
					row = png_malloc(png_p, sizeof(png_byte) * w * RGB_BYTES_PER_PIXEL);
					row_pointers[y] = row;
					
					for(x = 0; x < w; x++)
					{
						if((original_row_idx < qr_code->width) && (original_col_idx < qr_code->width))
						{
							if((x < 41))
							{
								*row++ = bpx->red;
								*row++ = bpx->green;
								*row++ = bpx->blue;
								original_col_idx = -1;
							} else {
								*row++ = (qr_code->data[qr_code->width * original_row_idx + original_col_idx] & 0x1) ? px->red : bpx->red;
								*row++ = (qr_code->data[qr_code->width * original_row_idx + original_col_idx] & 0x1) ? px->green : bpx->green;							
								*row++ = (qr_code->data[qr_code->width * original_row_idx + original_col_idx] & 0x1) ? px->blue : bpx->blue; 
							}
					
						} else {
							*row++ = bpx->red;
							*row++ = bpx->green;
							*row++ = bpx->blue;
						}
							
						if((x != 0) && ((x % PNG_SCALE_FACTOR) == 0)) original_col_idx++;
					}
					
					original_col_idx = 0;
					if((y != 0) && ((y % PNG_SCALE_FACTOR) == 0)) original_row_idx++;
				}
				
				for(y = 531; y < h; y++)
				{
					row = png_malloc(png_p, sizeof(png_byte) * w * RGB_BYTES_PER_PIXEL);
					row_pointers[y] = row;
					
					for(x = 0; x < w; x++)
					{
						*row++ = bpx->red;
						*row++ = bpx->green;
						*row++ = bpx->blue;
					}
				}
				
				/* Finish the write. */
				if(setjmp(png_jmpbuf(png_p)))
					return Qnil; /* More FAIL! */
				
				png_set_rows(png_p, png_write_info_p, row_pointers);
				png_write_png(png_p, png_write_info_p, PNG_TRANSFORM_IDENTITY, NULL);
			    png_write_end(png_p, png_write_info_p);
				png_destroy_write_struct(&png_p, &png_write_info_p);
			} else {
				printf("NOPE!\n");
			}
			
			fclose(fp);
		}
	}
	
	return Qnil;
}