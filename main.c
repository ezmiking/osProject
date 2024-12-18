#include <gtk/gtk.h>

void on_submit_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget **widgets = (GtkWidget **)user_data;

    const gchar *name = gtk_entry_get_text(GTK_ENTRY(widgets[0]));
    const gchar *budget = gtk_entry_get_text(GTK_ENTRY(widgets[2]));

    g_print("Name: %s\n", name);
    g_print("Maximum Budget: %s\n", budget);

    GtkListBox *order_list = GTK_LIST_BOX(widgets[1]);
    GList *rows = gtk_container_get_children(GTK_CONTAINER(order_list));
    for (GList *iter = rows; iter != NULL; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GList *cols = gtk_container_get_children(GTK_CONTAINER(row));
        const gchar *order = gtk_entry_get_text(GTK_ENTRY(cols->data));
        const gchar *quantity = gtk_entry_get_text(GTK_ENTRY(cols->next->data));
        g_print("Order: %s, Quantity: %s\n", order, quantity);
    }
    g_list_free(rows);
    g_print("Order submitted!\n");
}

void on_add_order_clicked(GtkButton *button, gpointer user_data) {
    GtkListBox *order_list = GTK_LIST_BOX(user_data);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *entry_order = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_order), "Order name");
    gtk_box_pack_start(GTK_BOX(row), entry_order, TRUE, TRUE, 0);

    GtkWidget *entry_quantity = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_quantity), "Quantity");
    gtk_box_pack_start(GTK_BOX(row), entry_quantity, TRUE, TRUE, 0);

    gtk_list_box_insert(order_list, row, -1);

    gtk_widget_show_all(GTK_WIDGET(order_list));
}

void on_add_user_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *new_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(new_window), "Order Page (New)");
    gtk_window_set_default_size(GTK_WINDOW(new_window), 400, 300);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(new_window), grid);

    GtkWidget *label_name = gtk_label_new("Enter your name:");
    gtk_grid_attach(GTK_GRID(grid), label_name, 0, 0, 1, 1);
    GtkWidget *entry_name = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    GtkWidget *button_add_user = gtk_button_new_with_label("Add User");
    gtk_widget_set_size_request(button_add_user, 80, 30);
    gtk_grid_attach(GTK_GRID(grid), button_add_user, 2, 0, 1, 1);

    GtkWidget *label_order = gtk_label_new("Orders:");
    gtk_grid_attach(GTK_GRID(grid), label_order, 0, 1, 1, 1);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_grid_attach(GTK_GRID(grid), scrolled_window, 1, 1, 1, 1);

    GtkWidget *order_list = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), order_list);

    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_grid_attach(GTK_GRID(grid), button_box, 2, 1, 1, 1);

    GtkWidget *button_add_order = gtk_button_new_with_label("Add Order");
    gtk_widget_set_size_request(button_add_order, 100, 30);
    gtk_box_pack_start(GTK_BOX(button_box), button_add_order, FALSE, FALSE, 0);

    GtkWidget *label_budget = gtk_label_new("Maximum Budget:");
    gtk_grid_attach(GTK_GRID(grid), label_budget, 0, 2, 1, 1);
    GtkWidget *entry_budget = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_budget, 1, 2, 1, 1);

    GtkWidget *button_submit = gtk_button_new_with_label("Submit Order");
    gtk_widget_set_size_request(button_submit, 80, 30);
    gtk_grid_attach(GTK_GRID(grid), button_submit, 2, 3, 1, 1);

    gtk_widget_show_all(new_window);
}

int main(int argc, char *argv[]) {
    GtkWidget *window, *grid, *entry_name, *entry_budget, *scrolled_window, *order_list, *button_submit, *button_add_order, *button_box, *button_add_user;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Order Page");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget *label_name = gtk_label_new("Enter your name:");
    gtk_grid_attach(GTK_GRID(grid), label_name, 0, 0, 1, 1);
    entry_name = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 0, 1, 1);

    button_add_user = gtk_button_new_with_label("Add User");
    gtk_widget_set_size_request(button_add_user, 80, 30);
    gtk_grid_attach(GTK_GRID(grid), button_add_user, 2, 0, 1, 1);
    g_signal_connect(button_add_user, "clicked", G_CALLBACK(on_add_user_clicked), NULL);

    GtkWidget *label_order = gtk_label_new("Orders:");
    gtk_grid_attach(GTK_GRID(grid), label_order, 0, 1, 1, 1);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_grid_attach(GTK_GRID(grid), scrolled_window, 1, 1, 1, 1);

    order_list = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), order_list);

    button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_grid_attach(GTK_GRID(grid), button_box, 2, 1, 1, 1);

    button_add_order = gtk_button_new_with_label("Add Order");
    gtk_widget_set_size_request(button_add_order, 100, 30);
    gtk_box_pack_start(GTK_BOX(button_box), button_add_order, FALSE, FALSE, 0);
    g_signal_connect(button_add_order, "clicked", G_CALLBACK(on_add_order_clicked), order_list);

    GtkWidget *label_budget = gtk_label_new("Maximum Budget:");
    gtk_grid_attach(GTK_GRID(grid), label_budget, 0, 2, 1, 1);
    entry_budget = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), entry_budget, 1, 2, 1, 1);

    button_submit = gtk_button_new_with_label("Submit Order");
    gtk_widget_set_size_request(button_submit, 80, 30);
    gtk_grid_attach(GTK_GRID(grid), button_submit, 2, 3, 1, 1);
    GtkWidget *widgets[3] = {entry_name, order_list, entry_budget};
    g_signal_connect(button_submit, "clicked", G_CALLBACK(on_submit_clicked), widgets);
    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}
