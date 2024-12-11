#include <gtk/gtk.h>

// تابع برای مدیریت کلیک روی دکمه ثبت سفارش
void on_submit_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget **widgets = (GtkWidget **)user_data;

    // دریافت نام کاربر
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(widgets[0]));
    // دریافت مقدار پول کاربر
    const gchar *budget = gtk_entry_get_text(GTK_ENTRY(widgets[2]));

    g_print("Name: %s\n", name);
    g_print("Maximum Budget: %s\n", budget);

    // پردازش لیست سفارش‌ها
    GtkListBox *order_list = GTK_LIST_BOX(widgets[1]);
    GList *rows = gtk_container_get_children(GTK_CONTAINER(order_list));
    for (GList *iter = rows; iter != NULL; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GList *cols = gtk_container_get_children(GTK_CONTAINER(row));

        if (cols && g_list_length(cols) == 2) {
            const gchar *order = gtk_entry_get_text(GTK_ENTRY(cols->data));
            const gchar *quantity = gtk_entry_get_text(GTK_ENTRY(g_list_nth_data(cols, 1)));
            g_print("Order: %s, Quantity: %s\n", order, quantity);
        }
    }
    g_list_free(rows);
    g_print("Order submitted!\n");
}

// تابع برای افزودن یک سفارش جدید به لیست
void on_add_order_clicked(GtkButton *button, gpointer user_data) {
    GtkListBox *order_list = GTK_LIST_BOX(user_data);

    // ایجاد یک ردیف جدید
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *entry_order = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_order), "Order name");
    gtk_box_pack_start(GTK_BOX(row), entry_order, TRUE, TRUE, 0);

    GtkWidget *entry_quantity = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_quantity), "Quantity");
    gtk_box_pack_start(GTK_BOX(row), entry_quantity, TRUE, TRUE, 0);

    // افزودن ردیف جدید به لیست
    gtk_list_box_insert(order_list, row, -1);

    // نمایش ردیف جدید
    gtk_widget_show_all(GTK_WIDGET(order_list));
}