#include <gtk/gtk.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib")

// ------------------ Global widgets ------------------
GtkWidget *text_view;
GtkWidget *entry;

// ------------------ Networking ------------------
SOCKET sockfd; // TCP socket

// ------------------ Thread-safe GUI update ------------------
gboolean update_text_view(gpointer user_data) {
    char *msg = (char *)user_data;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, msg, -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);
    g_free(msg);
    return FALSE;
}

// ------------------ Receiving messages ------------------
void *receive_messages_thread(void *arg) {
    char buffer[512];
    int n;
    while ((n = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        g_idle_add(update_text_view, g_strdup(buffer));
    }
    return NULL;
}

// ------------------ Send message ------------------
void send_message(GtkWidget *widget, gpointer data) {
    const gchar *msg = gtk_entry_get_text(GTK_ENTRY(entry));
    if (strlen(msg) == 0) return;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar *display_msg = g_strdup_printf("[You]: %s\n", msg);
    gtk_text_buffer_insert(buffer, &end, display_msg, -1);
    g_free(display_msg);

    // Scroll to end
    GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(gtk_widget_get_parent(text_view)));
    gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));

    send(sockfd, msg, (int)strlen(msg), 0);
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

// ------------------ Connect to server ------------------
int connect_to_server(const char *host, int port, const char *username, const char *password) {
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Resolve hostname
    struct hostent *he = gethostbyname(host);
    if (!he) {
        printf("DNS lookup failed for %s\n", host);
        return -1;
    }
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection Failed. WSAGetLastError=%d\n", WSAGetLastError());
        return -1;
    }

    // Send username and password
    send(sockfd, username, (int)strlen(username), 0);
    Sleep(100);
    send(sockfd, password, (int)strlen(password), 0);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages_thread, NULL);
    pthread_detach(recv_thread);

    return 0;
}

// ------------------ Text input dialogs ------------------
gchar *get_text_dialog(const char *title, const char *default_text, gboolean hide_text) {
    GtkWidget *dialog, *entry;
    gchar *result = NULL;

    dialog = gtk_dialog_new_with_buttons(title, NULL, GTK_DIALOG_MODAL,
                                         "_OK", GTK_RESPONSE_OK,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         NULL);
    entry = gtk_entry_new();
    if (hide_text)
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    if (default_text)
        gtk_entry_set_text(GTK_ENTRY(entry), default_text);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
        result = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    gtk_widget_destroy(dialog);
    return result;
}

// ------------------ Main GUI ------------------
int main(int argc, char *argv[]) {
    GtkWidget *window, *vbox, *scrolled_window, *button;

    gtk_init(&argc, &argv);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock initialization failed: %d\n", WSAGetLastError());
        return 1;
    }

    gchar *server_ip = get_text_dialog("Server Hostname", "0.tcp.in.ngrok.io", FALSE);
    if (!server_ip) return 0;

    gchar *port_str = get_text_dialog("Port", "15520", FALSE);
    if (!port_str) { g_free(server_ip); return 0; }
    int port = atoi(port_str);
    g_free(port_str);

    gchar *username = get_text_dialog("Username", "User", FALSE);
    if (!username) { g_free(server_ip); return 0; }

    gchar *password = get_text_dialog("Password", "", TRUE);
    if (!password) { g_free(server_ip); g_free(username); return 0; }

    if (connect_to_server(server_ip, port, username, password) < 0) {
        printf("Failed to connect to server.\n");
        g_free(server_ip);
        g_free(username);
        g_free(password);
        WSACleanup();
        return 1;
    }

    g_free(server_ip);
    g_free(username);
    g_free(password);

    // GTK Chat Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat App");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(send_message), NULL);
    g_signal_connect(entry, "activate", G_CALLBACK(send_message), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
