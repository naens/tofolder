#ifndef GST_H
#define GST_H

struct gst;


struct gst *new_gst();

void del_gst(struct gst *gst);

void print_tree(struct gst *gst);

void add_char(struct gst *gst, uint ch);

void add_string(struct gst *gst, uint *str);

void line_end(struct gst *gst);

void gst_strings(struct gst *gst, int *count, uint ***strings);

void longest_strings(struct gst *gst, int *count, uint ***strings);

#endif /* GST_H */
