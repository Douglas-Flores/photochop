#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdbool.h>
#include <math.h>

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
} Image;

// Componentes GTK
GtkBuilder *builder;
GtkWidget *window;
GtkWidget *image_picker;
GtkWidget *tone_quantity;

// Variáveis globais
Image original_img;
Image manipulated_img;
bool isImageLoaded = false;
GdkPixbuf *preview_pixbuf;

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
    
    preview_pixbuf = gdk_pixbuf_scale_simple(preview_pixbuf, manipulated_img.preview_width, manipulated_img.preview_height, GDK_INTERP_BILINEAR);

    gtk_image_set_from_pixbuf(GTK_IMAGE(manipulated_img.widget), preview_pixbuf);
}

void on_main_window_destroy(GtkWidget *widget, gpointer data) {
    if (isImageLoaded == true)
        close_files();
    gtk_main_quit();
}

void on_image_picker_file_set(GtkFileChooserButton *f) {
    original_img.name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(f));

    original_img.pixels = gdk_pixbuf_new_from_file(original_img.name, NULL);
    manipulated_img.pixels = gdk_pixbuf_new_from_file(original_img.name, NULL);
    preview_pixbuf = gdk_pixbuf_new_from_file(original_img.name, NULL);

    // defining original image
    original_img.width = gdk_pixbuf_get_width(original_img.pixels);
    original_img.height = gdk_pixbuf_get_height(original_img.pixels);
    original_img.aspect_ratio = (float)original_img.width / (float)original_img.height;
    original_img.preview_width = 428;
    original_img.preview_height = original_img.preview_width / original_img.aspect_ratio;
    // defining manipulated image
    manipulated_img.width = gdk_pixbuf_get_width(manipulated_img.pixels);
    manipulated_img.height = gdk_pixbuf_get_height(manipulated_img.pixels);
    manipulated_img.aspect_ratio = (float)manipulated_img.width / (float)manipulated_img.height;
    manipulated_img.preview_width = 428;
    manipulated_img.preview_height = original_img.preview_width / original_img.aspect_ratio;
    
    preview_pixbuf = gdk_pixbuf_scale_simple(preview_pixbuf, original_img.preview_width, original_img.preview_height, GDK_INTERP_BILINEAR);

    gtk_image_set_from_pixbuf(GTK_IMAGE(original_img.widget), preview_pixbuf);
    gtk_image_set_from_pixbuf(GTK_IMAGE(manipulated_img.widget), preview_pixbuf);
    
    printf("\n%s", original_img.name);
    load_files();
    copy_image();
}

void on_mirror_horizontal_clicked() {
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

    for (int y=0; y<manipulated_img.height; y++) {
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
    int width = gdk_pixbuf_get_width(manipulated_img.pixels);
    int height = gdk_pixbuf_get_height(manipulated_img.pixels);
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

    for (int y=0; y < (int)floor(height/2); y++) {
        // definindo ponteiro para a linha atual
        p_row = pixels + y * rowstride;
        // criando uma cópia da linha em um buffer
        row_buffer = malloc(rowstride*n_channels);
        memcpy(row_buffer, p_row, rowstride*n_channels);
        // pegando linha espelhada
        p_mirror_row = pixels + (height-y-1) * rowstride;
        // fazendo a troca
        for (int x=0; x < width; x++) {
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

void on_greyscale_clicked() {
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
        // definindo ponteiro para a linha a ser manipulada
        p_row = pixels + y * rowstride;
        // percorrendo a linha
        for (int x=0; x<width;x++){
            p = p_row + x * n_channels;
            guchar l = 0.299*p[0] + 0.587*p[1] + 0.114*p[2];
            p[0] = l;
            p[1] = l;
            p[2] = l;
            
        }
    }

    update_preview_image();

    printf("\n[OPERATION] Greyscale Filter");
}

void on_quantum_clicked(){
    // retrieving number of tones
    char tmp[4];
    sprintf(tmp, "%s", gtk_entry_get_text(GTK_ENTRY(tone_quantity)));
    int n = atoi(tmp);

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
        printf("\norra");
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

void on_histogram_button_clicked() {

}

void on_save_image_clicked() {
    FILE *saving_image;
    saving_image = fopen("images/saved.jpeg", "wb");
    fclose(saving_image);
    gdk_pixbuf_savev(manipulated_img.pixels, "images/saved.jpeg", "jpeg", NULL, NULL, NULL);

    printf("\n[SYS] Saved Image");
}

int main(int argc, char **argv) {

    // Inicializando componentes do GTK
    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_file("others/ui.glade");
    original_img.widget = GTK_WIDGET(gtk_builder_get_object(builder, "original_image"));
    manipulated_img.widget = GTK_WIDGET(gtk_builder_get_object(builder, "preview_image"));
    image_picker = GTK_WIDGET(gtk_builder_get_object(builder, "image_picker"));
    tone_quantity = GTK_WIDGET(gtk_builder_get_object(builder, "tone_quantity"));

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
        NULL
    );
    gtk_builder_connect_signals(builder, NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}