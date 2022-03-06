#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdbool.h>
#include <math.h>

#define WINDOW_WIDTH 428;

typedef struct {
    char *name;
    FILE *file;
    GdkPixbuf *pixels;
    GtkWidget *widget;
    int width;
    int height;
    int preview_width;
    int preview_height;
    float aspect_ratio;
    int greyed;
} Image;

// Componentes GTK
GtkBuilder *builder;
GtkWidget *window;
GtkWidget *window_histogram;
GtkWidget *image_picker;
GtkWidget *tone_quantity;
GtkWidget *canvas;
GtkAdjustment *brightness_adjustment;
GtkAdjustment *contrast_adjustment;

// Variáveis globais
Image original_img;
Image manipulated_img;
Image buffer_img;
bool isImageLoaded = false;
GdkPixbuf *preview_pixbuf;
int histogram_data[256];
int brightness_level = 0;
float contrast_level = 1;

int convolution_function(float kernel[3][3], uint8_t neighborhood[3][3]) {
    float output = 0;
    
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            output += kernel[y][x] * neighborhood[2-y][2-x];
        }
    }

    return (int) output;
}

void close_files(){
    if (isImageLoaded == true) {
        fclose(original_img.file);
        fclose(manipulated_img.file);
    }
}

void load_files() {
    if (isImageLoaded == true){
        close_files();
        isImageLoaded = false;
    }
    original_img.file = fopen(original_img.name, "rb");
    manipulated_img.file = fopen("images/temp.jpg", "wb");
    isImageLoaded = true;
}

void copy_image() {
    if (isImageLoaded == true){
        char buf[1];    // criando um buffer unitário

        // loop de leitura
        while (fread(buf,1,1,original_img.file) > 0)
            fwrite(buf,1,1,manipulated_img.file);// gravando 1 byte no novo arquivo
        fclose(manipulated_img.file);
    } else {
        printf("\n[ERROR] There is no loaded images!\n");
    }
}

void update_preview_image() {
    preview_pixbuf = gdk_pixbuf_copy(manipulated_img.pixels); 
    //preview_pixbuf = gdk_pixbuf_scale_simple(preview_pixbuf, manipulated_img.preview_width, manipulated_img.preview_height, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(manipulated_img.widget), preview_pixbuf);
}

void on_main_window_destroy(GtkWidget *widget, gpointer data) {
    if (isImageLoaded == true)
        close_files();
    gtk_main_quit();
}

void on_image_picker_file_set(GtkFileChooserButton *f) {
    original_img.name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(f));

    gtk_adjustment_set_value(brightness_adjustment, 0);
    gtk_adjustment_set_value(contrast_adjustment, 1);
    brightness_level = 0;
    contrast_level = 1;

    if (!original_img.name)
        return NULL;

    original_img.pixels = gdk_pixbuf_new_from_file(original_img.name, NULL);
    manipulated_img.pixels = gdk_pixbuf_new_from_file(original_img.name, NULL);
    preview_pixbuf = gdk_pixbuf_new_from_file(original_img.name, NULL);

    // defining original image
    original_img.width = gdk_pixbuf_get_width(original_img.pixels);
    original_img.height = gdk_pixbuf_get_height(original_img.pixels);
    original_img.aspect_ratio = (float)original_img.width / (float)original_img.height;
    original_img.preview_width = WINDOW_WIDTH;
    original_img.preview_height = original_img.preview_width / original_img.aspect_ratio;
    // defining manipulated image
    manipulated_img.width = gdk_pixbuf_get_width(manipulated_img.pixels);
    manipulated_img.height = gdk_pixbuf_get_height(manipulated_img.pixels);
    manipulated_img.aspect_ratio = (float)manipulated_img.width / (float)manipulated_img.height;
    manipulated_img.preview_width = WINDOW_WIDTH;
    manipulated_img.preview_height = original_img.preview_width / original_img.aspect_ratio;
    manipulated_img.greyed = 0;
    
    //preview_pixbuf = gdk_pixbuf_scale_simple(preview_pixbuf, original_img.preview_width, original_img.preview_height, GDK_INTERP_BILINEAR);

    gtk_image_set_from_pixbuf(GTK_IMAGE(original_img.widget), preview_pixbuf);
    gtk_image_set_from_pixbuf(GTK_IMAGE(manipulated_img.widget), preview_pixbuf);
    
    printf("\n%s", original_img.name);
    load_files();
    copy_image();
}

void on_mirror_horizontal_clicked() {
    if (isImageLoaded == false)
        return NULL;

    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    guchar *row_buffer;
    guchar *p_buffer;
    
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);

    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    for (int y = 0; y < manipulated_img.height; y++) {
        // definindo ponteiro para a linha a ser invertida
        p_row = pixels + y * rowstride;
        // criando uma cópia da linha em um buffer
        row_buffer = malloc(rowstride*n_channels);
        memcpy(row_buffer, p_row, rowstride*n_channels);
        // loop de inversão de linha
        for (int x = 0; x < manipulated_img.width; x++){
            p = p_row + x * n_channels;
            p_buffer = row_buffer + (manipulated_img.width - 1 - x) * n_channels;
            p[0] = p_buffer[0];
            p[1] = p_buffer[1];
            p[2] = p_buffer[2];
            
        }
        free(row_buffer);
    }

    update_preview_image();
    printf("\n[OPERATION] Horizontal Mirror");
}

void on_mirror_vertical_clicked() {
    if (isImageLoaded == false)
        return NULL;

    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    guchar *row_buffer;
    guchar *p_mirror_row, *p_mirror;
    guchar *p_buffer;
    
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);

    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    for (int y = 0; y < (int)floor(manipulated_img.height/2); y++) {
        // definindo ponteiro para a linha atual
        p_row = pixels + y * rowstride;
        // criando uma cópia da linha em um buffer
        row_buffer = malloc(rowstride*n_channels);
        memcpy(row_buffer, p_row, rowstride*n_channels);
        // pegando linha espelhada
        p_mirror_row = pixels + (manipulated_img.height-y-1) * rowstride;
        // fazendo a troca
        for (int x=0; x < manipulated_img.width; x++) {
            p = p_row + x * n_channels;
            p_mirror = p_mirror_row + x * n_channels;
            p_buffer = row_buffer + x * n_channels;
            p[0] = p_mirror[0];
            p[1] = p_mirror[1];
            p[2] = p_mirror[2];
            p_mirror[0] = p_buffer[0];
            p_mirror[1] = p_buffer[1];
            p_mirror[2] = p_buffer[2];
        }

        free(row_buffer);
    }

    update_preview_image();
    printf("\n[OPERATION] Vertical Mirror");
}

void on_brigthness_change() {
    if (isImageLoaded == false)
        return NULL;

    int brightness_adjustment_value = gtk_adjustment_get_value(brightness_adjustment);
    brightness_adjustment_value = brightness_adjustment_value - brightness_level;
    brightness_level = gtk_adjustment_get_value(brightness_adjustment);
    
    if (brightness_adjustment_value > 255 || brightness_adjustment_value < -255){
        printf("\n[OPERATION] Brightness \n [ERROR] Invalid brightness value");
        return NULL;
    }

    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);

    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    for (int y = 0; y < manipulated_img.height; y++) {
        // definindo ponteiro para a linha atual
        p_row = pixels + y * rowstride;
        // fazendo a troca
        for (int x=0; x < manipulated_img.width; x++) {
            p = p_row + x * n_channels;
            uint8_t new_value = (uint8_t) p[0] + (uint8_t) brightness_adjustment_value;

            if (brightness_adjustment_value > 0 && new_value < p[0])
                p[0] = 255;
            else if (brightness_adjustment_value < 0 && new_value > p[0])
                p[0] = 0;
            else
                p[0] = new_value;

            new_value = p[1] + (uint8_t) brightness_adjustment_value;
            if (brightness_adjustment_value > 0 && new_value < p[1])
                p[1] = 255;
            else if (brightness_adjustment_value < 0 && new_value > p[1])
                p[1] = 0;
            else
                p[1] = new_value;

            new_value = p[2] + (uint8_t) brightness_adjustment_value;
            if (brightness_adjustment_value > 0 && new_value < p[2])
                p[2] = 255;
            else if (brightness_adjustment_value < 0 && new_value > p[2])
                p[2] = 0;
            else
                p[2] = new_value;
        }
    }

    update_preview_image();
    printf("\n[OPERATION] Brightness");
}

void on_contrast_change() {
    if (isImageLoaded == false)
        return NULL;

    float contrast_adjustment_value = gtk_adjustment_get_value(contrast_adjustment);
    contrast_adjustment_value = contrast_adjustment_value * (1 / contrast_level);
    contrast_level = gtk_adjustment_get_value(contrast_adjustment);
    
    if (contrast_adjustment_value > 255 || contrast_adjustment_value < 0){
        printf("\n[OPERATION] Contrast \n [ERROR] Invalid contrast value");
        return NULL;
    }

    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);

    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    for (int y = 0; y < manipulated_img.height; y++) {
        // definindo ponteiro para a linha atual
        p_row = pixels + y * rowstride;
        // fazendo a troca
        for (int x=0; x < manipulated_img.width; x++) {
            p = p_row + x * n_channels;
            uint8_t new_value = (uint8_t) p[0] * contrast_adjustment_value;

            if (contrast_adjustment_value > 1 && new_value < p[0])
                p[0] = 255;
            else
                p[0] = new_value;

            new_value = (uint8_t) p[1] * contrast_adjustment_value;
            if (contrast_adjustment_value > 1 && new_value < p[1])
                p[1] = 255;
            else
                p[1] = new_value;

            new_value = (uint8_t) p[2] * contrast_adjustment_value;
            if (contrast_adjustment_value > 1 && new_value < p[2])
                p[2] = 255;
            else
                p[2] = new_value;
        }
    }

    update_preview_image();
    printf("\n[OPERATION] Contrast");
}

void on_negative_clicked() {
    if (isImageLoaded == false)
        return NULL;

    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);

    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    for (int y = 0; y < manipulated_img.height; y++) {
        // definindo ponteiro para a linha atual
        p_row = pixels + y * rowstride;
        // fazendo a troca
        for (int x=0; x < manipulated_img.width; x++) {
            p = p_row + x * n_channels;
            p[0] = (uint8_t) 255 - (uint8_t) p[0];
            p[1] = (uint8_t) 255 - (uint8_t) p[1];
            p[2] = (uint8_t) 255 - (uint8_t) p[2];
        }
    }

    update_preview_image();
    printf("\n[OPERATION] Negative");
}

void greyscale(Image *img) {
    if (isImageLoaded == false)
        return NULL;

    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    
    n_channels = gdk_pixbuf_get_n_channels(img->pixels);

    g_assert (gdk_pixbuf_get_colorspace (img->pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (img->pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (img->pixels);
    pixels = gdk_pixbuf_get_pixels (img->pixels);

    for (int y = 0; y < img->height; y++) {
        // definindo ponteiro para a linha a ser manipulada
        p_row = pixels + y * rowstride;
        // percorrendo a linha
        for (int x = 0; x < img->width; x++){
            p = p_row + x * n_channels;
            guchar l = 0.299*p[0] + 0.587*p[1] + 0.114*p[2];
            p[0] = l;
            p[1] = l;
            p[2] = l;
        }
    }

    img->greyed = 1;
    update_preview_image();
}

void on_greyscale_clicked() {
    greyscale(&manipulated_img);
    printf("\n[OPERATION] Greyscale Filter");
}

void on_quantum_clicked(){
    if (isImageLoaded == false)
        return NULL;

    // retrieving number of tones
    char tmp[4];
    sprintf(tmp, "%s", gtk_entry_get_text(GTK_ENTRY(tone_quantity)));
    int n = atoi(tmp);

    if (n == 0 || !n)
        return NULL;

    // Tone mapping
    guchar t1=0, t2=0;
    int tam_int = 0;

    int width = gdk_pixbuf_get_width(manipulated_img.pixels);
    int height = gdk_pixbuf_get_height(manipulated_img.pixels);
    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);

    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);

    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    for (int y=0; y<height; y++) {
        p_row = pixels + y * rowstride;
        for (int x=0; x<width;x++){
            p = p_row + x * n_channels;
            guchar tx = p[0];
            if (tx < t1)
                t1 = tx;
            if (tx > t2)
                t2 = tx;
        }
    }

    tam_int = t2 - t1 + 1;

    if(n < tam_int){
        int tb = (int)(tam_int/n);
        for (int y=0; y<height; y++) {
            p_row = pixels + y * rowstride;
            for (int x=0; x<width;x++){
                p = p_row + x * n_channels;
                guchar bin = (guchar)(p[0] / tb);
                guchar bin_center = (guchar)(t1-0.5+bin*tb + tb/2);
                p[0] = bin_center;
                p[1] = bin_center;
                p[2] = bin_center;
            }
        }
    }

    update_preview_image();

    printf("\n[OPERATION] Tone Mapping");

}

gboolean draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data) {
    guint width = 1000, height = 500;

    // Desenhando eixo y
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, 20, floor(height*0.05));
    cairo_line_to(cr, 20, height - 20);
    cairo_stroke(cr);

    // Desenhando eixo x
    cairo_move_to(cr, 20, height - 20);
    cairo_line_to(cr, floor(width*0.95), height - 20);
    cairo_stroke(cr);

    // Desenhando o histograma
    cairo_set_source_rgb(cr, 0.5, 0.03, 0.03);
    cairo_set_line_width(cr, 2);
    float spacing_x = (width*0.95 - 20.0) / 256.0;
    int max_histogram = 0;
    // Pegando o valor máximo do histograma
    for(int i = 0; i < 256; i++)
        if (histogram_data[i] > max_histogram)
            max_histogram = histogram_data[i];

    // Calculando a escala em pixels
    float base_y = height - 20;
    float top_y = height*0.05;
    float range_y = base_y - top_y;

    // Imprimindo os dados na tela
    for(int i = 0; i < 256; i++){
        float x = 21.0 + spacing_x * i;
        float data_score = 1 - ((float)histogram_data[i] / (float)max_histogram);
        int top = top_y + data_score * range_y;
        cairo_move_to(cr, x, (guint)top);
        cairo_line_to(cr, x, (guint)base_y);
        cairo_stroke(cr);
    }

    return FALSE;
}

void compute_histogram(Image *img, int *hist) {
    int rowstride, n_channels;
    guchar *pixels, *p_row, *p;
    
    // Inicializando array do histograma
    for (int i = 0; i < 256; i++) hist[i] = 0;
    
    n_channels = gdk_pixbuf_get_n_channels(img->pixels);
    g_assert (gdk_pixbuf_get_colorspace (img->pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (img->pixels) == 8);

    rowstride = gdk_pixbuf_get_rowstride (img->pixels);
    pixels = gdk_pixbuf_get_pixels (img->pixels);
    
    // preparando o buffer ---------------------
    buffer_img = *img;
    buffer_img.pixels = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace(manipulated_img.pixels),
        false,
        8,
        buffer_img.width,
        buffer_img.height
    );

    int buffer_rowstride;
    guchar *buffer_pixels, *buffer_row, *buffer_pixel;
    g_assert (gdk_pixbuf_get_colorspace (buffer_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (buffer_img.pixels) == 8);
    g_assert (n_channels == 3);
    buffer_rowstride = gdk_pixbuf_get_rowstride (buffer_img.pixels);
    buffer_pixels = gdk_pixbuf_get_pixels (buffer_img.pixels);

    for (int y = 0; y < buffer_img.height; y++) {
        p_row = pixels + y * rowstride;
        buffer_row = buffer_pixels + y * buffer_rowstride;
        for (int x = 0; x < buffer_img.width; x++) {
            p = p_row + x * n_channels;
            buffer_pixel = buffer_row + x * n_channels;
            buffer_pixel[0] = p[0];
            buffer_pixel[1] = p[1];
            buffer_pixel[2] = p[2];
        }   
    }
    // -----------------------------------------

    if (n_channels == 3){
        if (img->greyed != 1) greyscale(img);
        g_assert (n_channels == 3);
    }
    else if (n_channels == 1) {
        g_assert (n_channels == 1);
    }
    else {
        printf("\n[ERROR] Cannot read number of channels from image");
        return NULL;
    }

    for (int y = 0; y < img->height; y++) {
        // definindo ponteiro para a linha a ser manipulada
        p_row = pixels + y * rowstride;
        // percorrendo a linha
        for (int x = 0; x < img->width; x++) {
            p = p_row + x * n_channels;
            int index = 0;

            if (p[0] > 255)
                index = 255;
            else if (p[0] < 0)
                index = 0;
            else
                index = p[0];

            hist[index]++;
        }
    }

    manipulated_img = buffer_img;
}

void on_histogram_button_clicked() {
    if (isImageLoaded == false)
        return NULL;

    compute_histogram(&manipulated_img, histogram_data);

    canvas = GTK_WIDGET(gtk_builder_get_object(builder, "histogram_draw"));
    gtk_widget_show_all(window_histogram);
    g_signal_connect (G_OBJECT(canvas), "draw", G_CALLBACK(draw_callback), NULL);
    printf("\n[FUNCTION] Show Histogram");
}

void on_histogram_equalize_button_clicked() {
    int histogram[256];
    int hist_cum[256];
    for (int i = 0; i < 256; i++) hist_cum[i] = 0;
    float alpha = 255.0 / (manipulated_img.height * manipulated_img.width);

    compute_histogram(&manipulated_img, histogram);
    hist_cum[0] = alpha * histogram[0];
    
    for (int i = 1; i < 256; i++){
        printf("\n%d", histogram[i]);
        hist_cum[i] = hist_cum[i-1] + alpha * histogram[i];
    }
    
    int rowstride, n_channels;
    guchar *pixels, *row, *p;

    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);
    rowstride = gdk_pixbuf_get_rowstride(manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels(manipulated_img.pixels);
    
    for (int y = 0; y < manipulated_img.height; y++) {
        row = pixels + y * rowstride;
        for (int x = 0; x < manipulated_img.width; x++) {
            p = row + x * n_channels;
            p[0] = (uint8_t) hist_cum[p[0]];
            if (n_channels == 3) {
                p[1] = (uint8_t) hist_cum[p[1]];
                p[2] = (uint8_t) hist_cum[p[2]];
            }
        }   
    }

    update_preview_image();
    printf("\n[OPERATION] Histogram Equalization");
}

void on_save_image_clicked() {
    FILE *saving_image;
    saving_image = fopen("images/saved.jpeg", "wb");
    fclose(saving_image);
    gdk_pixbuf_savev(manipulated_img.pixels, "images/saved.jpeg", "jpeg", NULL, NULL, NULL);

    printf("\n[SYS] Saved Image");
}

void on_histogram_window_destroy() {
    gtk_widget_destroy(window_histogram);
}

void on_clock_wise_clicked() {
    if (isImageLoaded == false)
        return NULL;
    
    // preparando o buffer ---------------------
    buffer_img = manipulated_img;
    buffer_img.height = manipulated_img.width;
    buffer_img.width = manipulated_img.height;
    buffer_img.aspect_ratio = 1 / buffer_img.aspect_ratio;
    buffer_img.preview_height = buffer_img.preview_width / buffer_img.aspect_ratio;
    buffer_img.pixels = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace (manipulated_img.pixels),
        false,
        8,
        buffer_img.width,
        buffer_img.height
    );
    // -----------------------------------------

    // preparando acesso ao pixels -------------------------------------------------------
    int rowstride, n_channels;
    guchar *pixels, *row, *p;
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);
    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);
    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);
    // ----------------------------------------------------------------------------------

    // preparando acesso ao buffer ------------------------------------------------------
    int buffer_rowstride;
    guchar *buffer_pixels, *buffer_row, *buffer_pixel;
    g_assert (gdk_pixbuf_get_colorspace (buffer_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (buffer_img.pixels) == 8);
    g_assert (n_channels == 3);
    buffer_rowstride = gdk_pixbuf_get_rowstride (buffer_img.pixels);
    buffer_pixels = gdk_pixbuf_get_pixels (buffer_img.pixels);
    // ----------------------------------------------------------------------------------

    for (int y = 0; y < manipulated_img.height; y++) {
        // definindo ponteiro para a linha a ser rotacionada
        row = pixels + y * rowstride;
        // loop de rotação
        for (int x = 0; x < manipulated_img.width; x++){
            p = row + x * n_channels;
            buffer_row = buffer_pixels + x * buffer_rowstride;
            buffer_pixel = buffer_row + (buffer_img.width - 1 - y) * n_channels;
            buffer_pixel[0] = p[0];
            buffer_pixel[1] = p[1];
            buffer_pixel[2] = p[2];
        }
    }
    manipulated_img = buffer_img;
    printf("\n width: %d \n height: %d\n aspect_ratio: %.2f", manipulated_img.width, manipulated_img.height, manipulated_img.aspect_ratio);
    update_preview_image();
    printf("\n[OPERATION] Clockwise Rotation");
}

void on_counter_clock_wise_button_clicked() {
    if (isImageLoaded == false)
        return NULL;
    
    // preparando o buffer ---------------------
    buffer_img = manipulated_img;
    buffer_img.height = manipulated_img.width;
    buffer_img.width = manipulated_img.height;
    buffer_img.aspect_ratio = 1 / buffer_img.aspect_ratio;
    buffer_img.preview_height = buffer_img.preview_width / buffer_img.aspect_ratio;
    buffer_img.pixels = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace (manipulated_img.pixels),
        false,
        8,
        buffer_img.width,
        buffer_img.height
    );
    // -----------------------------------------

    // preparando acesso ao pixels -------------------------------------------------------
    int rowstride, n_channels;
    guchar *pixels, *row, *p;
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);
    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);
    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);
    // ----------------------------------------------------------------------------------

    // preparando acesso ao buffer ------------------------------------------------------
    int buffer_rowstride;
    guchar *buffer_pixels, *buffer_row, *buffer_pixel;
    g_assert (gdk_pixbuf_get_colorspace (buffer_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (buffer_img.pixels) == 8);
    g_assert (n_channels == 3);
    buffer_rowstride = gdk_pixbuf_get_rowstride (buffer_img.pixels);
    buffer_pixels = gdk_pixbuf_get_pixels (buffer_img.pixels);
    // ----------------------------------------------------------------------------------

    for (int y = manipulated_img.height-1; y >= 0; y--) {
        // definindo ponteiro para a linha a ser rotacionada
        row = pixels + y * rowstride;
        // loop de rotação
        for (int x = 0; x < manipulated_img.width; x++){
            p = row + x * n_channels;
            buffer_row = buffer_pixels + (manipulated_img.width - x - 1) * buffer_rowstride;
            buffer_pixel = buffer_row + (y) * n_channels;
            buffer_pixel[0] = p[0];
            buffer_pixel[1] = p[1];
            buffer_pixel[2] = p[2];
        }
    }
    manipulated_img = buffer_img;
    update_preview_image();
    printf("\n[OPERATION] Counterclockwise Rotation");
}

void on_zoom_out_button_clicked() {
    if (isImageLoaded == false)
        return NULL;
    
    // preparando o buffer ---------------------
    buffer_img = manipulated_img;
    buffer_img.height = ceil(manipulated_img.height / 2);
    buffer_img.width = ceil(manipulated_img.width / 2);
    buffer_img.preview_height = buffer_img.preview_width / buffer_img.aspect_ratio;
    buffer_img.pixels = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace (manipulated_img.pixels),
        false,
        8,
        buffer_img.width,
        buffer_img.height
    );
    // -----------------------------------------

    // preparando acesso ao pixels -------------------------------------------------------
    int rowstride, n_channels;
    guchar *pixels, *row, *p;
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);
    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);
    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);
    // ----------------------------------------------------------------------------------

    // preparando acesso ao buffer ------------------------------------------------------
    int buffer_rowstride;
    guchar *buffer_pixels, *buffer_row, *buffer_pixel;
    g_assert (gdk_pixbuf_get_colorspace (buffer_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (buffer_img.pixels) == 8);
    g_assert (n_channels == 3);
    buffer_rowstride = gdk_pixbuf_get_rowstride (buffer_img.pixels);
    buffer_pixels = gdk_pixbuf_get_pixels (buffer_img.pixels);
    // ----------------------------------------------------------------------------------

    guchar* p_ret[2][2];
    int buf_y = 0;
    int buf_x = 0;
    for (int y = 0; y < manipulated_img.height; y+=2) {
        buffer_row = buffer_pixels + buf_y * buffer_rowstride;
        // loop de redução
        for (int x = 0; x < manipulated_img.width; x+=2){
            int square_height = 1;
            int square_width = 1;
            
            row = pixels + y * rowstride;

            p = row + x * n_channels;
            
            p_ret[0][0] = row + x * n_channels;
            if (row + (x+1) * n_channels){
                p_ret[0][1] = row + (x+1) * n_channels;
                square_width = 2;
            }

            if (pixels + (y+1) * rowstride) {
                square_height = 2;
                row = pixels + (y+1) * rowstride;
                p_ret[1][0] = row + x * n_channels;
                if (row + (x+1) * n_channels)
                    p_ret[1][1] = row + (x+1) * n_channels;
            }
            
            buffer_pixel = buffer_row + buf_x * n_channels;
            
            int R = 0, G = 0, B = 0;
            
            for (int i = 0; i < square_height; i++) {
                for (int j = 0; j < square_width; j++) {
                    p = p_ret[i][j];
                    R += p[0];
                    if (n_channels == 3) {
                        G += p[1];
                        B += p[2];
                    }
                }
            }
            
            buffer_pixel[0] = (uint8_t) (R / 4);
            if (n_channels == 3) {
                buffer_pixel[1] = (uint8_t) (G / 4);
                buffer_pixel[2] = (uint8_t) (B / 4);
            }

            buf_x++;
        }
        buf_x = 0;
        buf_y++;
    }
    manipulated_img = buffer_img;
    printf("\n width: %d \n height: %d\n aspect_ratio: %.2f", manipulated_img.width, manipulated_img.height, manipulated_img.aspect_ratio);
    update_preview_image();
    printf("\n[OPERATION] Zoom Out");
}

void on_zoom_in_button_clicked() {
    if (isImageLoaded == false)
        return NULL;
    
    // preparando o buffer ---------------------
    buffer_img = manipulated_img;
    buffer_img.height = manipulated_img.height * 2;
    buffer_img.width = manipulated_img.width * 2;
    buffer_img.preview_height = buffer_img.preview_width / buffer_img.aspect_ratio;
    buffer_img.pixels = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace (manipulated_img.pixels),
        false,
        8,
        buffer_img.width,
        buffer_img.height
    );
    // -----------------------------------------

    // preparando acesso ao pixels -------------------------------------------------------
    int rowstride, n_channels;
    guchar *pixels, *row, *p;
    n_channels = gdk_pixbuf_get_n_channels(manipulated_img.pixels);
    g_assert (gdk_pixbuf_get_colorspace (manipulated_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (manipulated_img.pixels) == 8);
    g_assert (n_channels == 3);
    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);
    // ----------------------------------------------------------------------------------

    // preparando acesso ao buffer ------------------------------------------------------
    int buffer_rowstride;
    guchar *buffer_pixels, *buffer_row, *buffer_pixel;
    g_assert (gdk_pixbuf_get_colorspace (buffer_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (buffer_img.pixels) == 8);
    g_assert (n_channels == 3);
    buffer_rowstride = gdk_pixbuf_get_rowstride (buffer_img.pixels);
    buffer_pixels = gdk_pixbuf_get_pixels (buffer_img.pixels);
    // ----------------------------------------------------------------------------------

    // Passo 1: Espaçando os pixels -----------------------------------------------------
    int buf_y = 0;
    int buf_x = 0;
    for (int y = 0; y < manipulated_img.height; y++) {
        row = pixels + y * rowstride;
        buffer_row = buffer_pixels + buf_y * buffer_rowstride;

        for (int x = 0; x < manipulated_img.width; x++){
            p = row + x * n_channels;
            buffer_pixel = buffer_row + buf_x * n_channels;

            buffer_pixel[0] = p[0];
            if (n_channels == 3) {
                buffer_pixel[1] = p[1];
                buffer_pixel[2] = p[2];
            }

            buf_x += 2;
        }
        buf_x = 0;
        buf_y += 2;
    }
    // ----------------------------------------------------------------------------------
    // Passo 2: Preenchendo colunas -----------------------------------------------------
    for (int y = 0; y < buffer_img.height; y+=2) {
        buffer_row = buffer_pixels + y * buffer_rowstride;

        for (int x = 1; x < buffer_img.width; x+=2){
            guchar *prev, *next;
            prev = buffer_row + (x-1) * n_channels;
            buffer_pixel = buffer_row + x * n_channels;
            next = buffer_row + (x+1) * n_channels;
            
            buffer_pixel[0] = (uint8_t) ((prev[0] + next[0]) / 2);
            if (n_channels == 3) {
                buffer_pixel[1] = (uint8_t) ((prev[1] + next[1]) / 2);
                buffer_pixel[2] = (uint8_t) ((prev[2] + next[2]) / 2);
            }
        }
    }
    // ----------------------------------------------------------------------------------
    // Passo 3: Preenchendo linhas ------------------------------------------------------
    for (int y = 1; y < buffer_img.height; y+=2) {
        guchar *prev_row = buffer_pixels + (y-1) * buffer_rowstride;
        buffer_row = buffer_pixels + y * buffer_rowstride;
        guchar *next_row = buffer_pixels + (y+1) * buffer_rowstride;

        for (int x = 0; x < buffer_img.width; x++){
            guchar *prev, *next;
            prev = prev_row + x * n_channels;
            buffer_pixel = buffer_row + x * n_channels;
            next = next_row + x * n_channels;
            
            buffer_pixel[0] = (uint8_t) ((prev[0] + next[0]) / 2);
            if (n_channels == 3) {
                buffer_pixel[1] = (uint8_t) ((prev[1] + next[1]) / 2);
                buffer_pixel[2] = (uint8_t) ((prev[2] + next[2]) / 2);
            }
        }
    }
    
    manipulated_img = buffer_img;
    update_preview_image();
    printf("\n[OPERATION] Zoom In");
}

void convolution(float kernel[3][3]) {
    if (isImageLoaded == false)
        return NULL;

    int rowstride, n_channels;
    guchar *pixels;
    n_channels = gdk_pixbuf_get_n_channels(original_img.pixels);
    g_assert (gdk_pixbuf_get_colorspace (original_img.pixels) == GDK_COLORSPACE_RGB);
    g_assert (gdk_pixbuf_get_bits_per_sample (original_img.pixels) == 8);

    if (n_channels == 3){
        greyscale(&manipulated_img);
        g_assert (n_channels == 3);
    }
    else if (n_channels == 1)
        g_assert (n_channels == 1);
    else {
        printf("\n[ERROR] Cannot read number of channels from image");
        return NULL;
    }
    rowstride = gdk_pixbuf_get_rowstride (manipulated_img.pixels);
    pixels = gdk_pixbuf_get_pixels (manipulated_img.pixels);

    // preparando o buffer ---------------------
    buffer_img = manipulated_img;
    buffer_img.pixels = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace (manipulated_img.pixels),
        false,
        8,
        buffer_img.width,
        buffer_img.height
    );
    int buffer_rowstride;
    guchar *buffer_pixels, *buffer_row, *buffer_p;
    buffer_rowstride = gdk_pixbuf_get_rowstride(buffer_img.pixels);
    buffer_pixels = gdk_pixbuf_get_pixels(buffer_img.pixels);
    // -----------------------------------------

    for (int y = 1; y < manipulated_img.height-1; y++) {
        // definindo ponteiro para a linha a ser manipulada
        buffer_row = buffer_pixels + y * buffer_rowstride;
        // percorrendo a linha
        for (int x = 1; x < manipulated_img.width-1; x++){
            buffer_p = buffer_row + x * n_channels;
            // building the matrix -------------------------------------------------
            uint8_t neighborhood[3][3];
            for (int _y = 0; _y < 3; _y++) {
                guchar *p_newrow = pixels + (y - (1 - _y)) * rowstride;
                for (int _x = 0; _x < 3; _x++) {
                    neighborhood[_y][_x] = (p_newrow + (x - (1 - _x)) * n_channels)[0];
                }
            }
            //----------------------------------------------------------------------

            // Aplying Convolution
            int pixel_value = convolution_function(kernel, neighborhood);

            if (n_channels == 1) {
                if (pixel_value > 255)
                    buffer_p[0] = (uint8_t) 255;
                else if (pixel_value < 0)
                    buffer_p[0] = (uint8_t) 0;
                else
                    buffer_p[0] = (uint8_t) pixel_value;
            }
            
            else if (n_channels == 3) {
                if (pixel_value > 255)
                    buffer_p[0] = (uint8_t) 255;
                else if (pixel_value < 0)
                    buffer_p[0] = (uint8_t) 0;
                else
                    buffer_p[0] = (uint8_t) pixel_value;
                
                if (pixel_value > 255)
                    buffer_p[1] = (uint8_t) 255;
                else if (pixel_value < 0)
                    buffer_p[1] = (uint8_t) 0;
                else
                    buffer_p[1] = (uint8_t) pixel_value;
                
                if (pixel_value > 255)
                    buffer_p[2] = (uint8_t) 255;
                else if (pixel_value < 0)
                    buffer_p[2] = (uint8_t) 0;
                else
                    buffer_p[2] = (uint8_t) pixel_value;
            }
        }
    }

    manipulated_img = buffer_img;
    update_preview_image();
}

void on_gaussian_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = 0.0625;
    kernel[0][1] = 0.125;
    kernel[0][2] = 0.0625;
    kernel[1][0] = 0.125;
    kernel[1][1] = 0.25;
    kernel[1][2] = 0.125;
    kernel[2][0] = 0.0625;
    kernel[2][1] = 0.125;
    kernel[2][2] = 0.0625;

    convolution(kernel);

    printf("\n[OPERATION] Gaussian Filter");
}

void on_laplacian_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = 0;
    kernel[0][1] = -1;
    kernel[0][2] = 0;
    kernel[1][0] = -1;
    kernel[1][1] = 4;
    kernel[1][2] = -1;
    kernel[2][0] = 0;
    kernel[2][1] = -1;
    kernel[2][2] = 0;

    convolution(kernel);
    printf("\n[OPERATION] Laplacian Filter");
}

void on_prewitt_hx_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = -1;
    kernel[0][1] = 0;
    kernel[0][2] = 1;
    kernel[1][0] = -1;
    kernel[1][1] = 0;
    kernel[1][2] = 1;
    kernel[2][0] = -1;
    kernel[2][1] = 0;
    kernel[2][2] = 1;

    convolution(kernel);
    printf("\n[OPERATION] Prewitt Hx");
}

void on_prewitt_hy_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = -1;
    kernel[0][1] = -1;
    kernel[0][2] = -1;
    kernel[1][0] = 0;
    kernel[1][1] = 0;
    kernel[1][2] = 0;
    kernel[2][0] = 1;
    kernel[2][1] = 1;
    kernel[2][2] = 1;

    convolution(kernel);
    printf("\n[OPERATION] Prewitt Hy");
}

void on_high_pass_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = -1;
    kernel[0][1] = -1;
    kernel[0][2] = -1;
    kernel[1][0] = -1;
    kernel[1][1] = 8;
    kernel[1][2] = -1;
    kernel[2][0] = -1;
    kernel[2][1] = -1;
    kernel[2][2] = -1;

    convolution(kernel);
    printf("\n[OPERATION] High Pass");
}

void on_sobel_hx_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = -1;
    kernel[0][1] = 0;
    kernel[0][2] = 1;
    kernel[1][0] = -2;
    kernel[1][1] = 0;
    kernel[1][2] = 2;
    kernel[2][0] = -1;
    kernel[2][1] = 0;
    kernel[2][2] = 1;

    convolution(kernel);
    printf("\n[OPERATION] Sobel Hx");
}

void on_sobel_hy_button_clicked() {
    float kernel[3][3];

    kernel[0][0] = -1;
    kernel[0][1] = -2;
    kernel[0][2] = -1;
    kernel[1][0] = 0;
    kernel[1][1] = 0;
    kernel[1][2] = 0;
    kernel[2][0] = 1;
    kernel[2][1] = 2;
    kernel[2][2] = 1;

    convolution(kernel);
    printf("\n[OPERATION] Sobel Hx");
}

int main(int argc, char **argv) {

    // Inicializando componentes do GTK
    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file("others/ui.glade");
    original_img.widget = GTK_WIDGET(gtk_builder_get_object(builder, "original_image"));
    manipulated_img.widget = GTK_WIDGET(gtk_builder_get_object(builder, "preview_image"));
    image_picker = GTK_WIDGET(gtk_builder_get_object(builder, "image_picker"));
    tone_quantity = GTK_WIDGET(gtk_builder_get_object(builder, "tone_quantity"));
    brightness_adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "brightness_adjustment"));
    contrast_adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "contrast_adjustment"));

    gtk_builder_add_callback_symbols(
        builder,
        "on_main_window_destroy", G_CALLBACK(on_main_window_destroy),
        "on_image_picker_file_set", G_CALLBACK(on_image_picker_file_set),
        "on_mirror_horizontal_clicked", G_CALLBACK(on_mirror_horizontal_clicked),
        "on_mirror_vertical_clicked", G_CALLBACK(on_mirror_vertical_clicked),
        "on_greyscale_clicked", G_CALLBACK(on_greyscale_clicked),
        "on_quantum_clicked", G_CALLBACK(on_quantum_clicked),
        "on_save_image_clicked", G_CALLBACK(on_save_image_clicked),
        "on_histogram_button_clicked", G_CALLBACK(on_histogram_button_clicked),
        "on_histogram_window_destroy", G_CALLBACK(on_histogram_window_destroy),
        "on_brightness_adjustment_value_changed", G_CALLBACK(on_brigthness_change),
        "on_negative_button_clicked", G_CALLBACK(on_negative_clicked),
        "on_contrast_adjustment_value_changed", G_CALLBACK(on_contrast_change),
        "on_clock_wise_button_clicked", G_CALLBACK(on_clock_wise_clicked),
        "on_counterclock_wise_button_clicked", G_CALLBACK(on_counter_clock_wise_button_clicked),
        "on_zoom_in_button_clicked", G_CALLBACK(on_zoom_in_button_clicked),
        "on_zoom_out_button_clicked", G_CALLBACK(on_zoom_out_button_clicked),
        "on_gaussian_button_clicked", G_CALLBACK(on_gaussian_button_clicked),
        "on_laplacian_button_clicked", G_CALLBACK(on_laplacian_button_clicked),
        "on_prewitt_hx_button_clicked", G_CALLBACK(on_prewitt_hx_button_clicked),
        "on_prewitt_hy_button_clicked", G_CALLBACK(on_prewitt_hy_button_clicked),
        "on_high_pass_button_clicked", G_CALLBACK(on_high_pass_button_clicked),
        "on_sobel_hx_button_clicked", G_CALLBACK(on_sobel_hx_button_clicked),
        "on_sobel_hy_button_clicked", G_CALLBACK(on_sobel_hy_button_clicked),
        "on_histogram_equalize_button_clicked", G_CALLBACK(on_histogram_equalize_button_clicked),
        NULL
    );
    gtk_builder_connect_signals(builder, NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    window_histogram = GTK_WIDGET(gtk_builder_get_object(builder, "histogram_window"));

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
