#ifndef GST_H
#define GST_H

#include <stdint.h>

struct gst;


struct gst *new_gst();

void del_gst(struct gst *gst);

void print_tree(struct gst *gst);

void add_char(struct gst *gst, uint32_t ch);

void add_string(struct gst *gst, uint32_t *str);

void line_end(struct gst *gst);

void gst_strings(struct gst *gst, int *count, uint32_t ***strings);

void longest_strings(struct gst *gst, int *count, uint32_t ***strings);

#endif /* GST_H */
