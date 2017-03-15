#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <dirent.h>

static char cpu_ratio[50];
static char mem_ratio[50];
static long idle, total;       //计算cpu时的数据
static int flag1 = 0;          //计算cpu使用率时启动程序的标志
static int flag2 = 0;          //计算单个进程cpu使用率时使用的标志

static long mem_size;
static long mem_free;
static float cpu_used_ratio = 0;         //cpu使用率
static float cpu_data[500];              //cpu历史数据
static int flag3 = 0;                    //初始化cpu_data数组中数据的标志
static int cpu_first_data = 0;           //最早的数据，下一个要淘汰的数据
static float mem_data[500];              //mem历史数据
static int flag4 = 0;                    //初始化mem_data数组中数据的标志
static int mem_first_data = 0;           //最早的数据，下一个要淘汰的数据

static GtkWidget *cpu_drawing_area;
static GtkWidget *mem_drawing_area;
static GtkWidget *note;

char *pid1 = NULL;

void reboot_computer(GtkButton *button, gpointer user_data)
{
    system("reboot");
}

char *get_hostname(char *str_1)
{
    FILE *buf;
    int i = 0;
    char *string = str_1;
    buf = fopen("/proc/sys/kernel/hostname", "r");
    fread(string, 1, 9, buf);
    string[10] = '\0';
    fclose(buf);
    return string;
}

char *get_time1(char *str_1)
{
    FILE *buf;
    int i = 0;
    long timee;
    int hour_, min_, sec_;
    char *string = str_1;
    char *t;
    buf = fopen("/proc/uptime", "r");
    fread(string, 1, 8, buf);
    string[9] = '\0';
    timee = atoi(string);
    hour_ = timee / 3600;
    min_ = (timee - hour_ * 3600) / 60;
    sec_ = timee % 60;
    t = (char *)malloc(100);
    sprintf(t, "%d:%d:%d", hour_, min_, sec_);
    fclose(buf);
    return t;
}

char *get_time2(char *str_1)
{
    FILE *buf;
    int i = 0, j = 0;
    int timee;
    int hour_, min_, sec_;
    char *string = str_1, *str, *t;
    str = (char *)malloc(sizeof(char) * 8);
    buf = fopen("/proc/uptime", "r");
    fread(string, 1, 16, buf);
    string[16] = '\0';
    for (i = 8; i < 17; ++i)
    {
        str[j] = string[i];
        j++;
    }
    timee = atoi(str);
    timee = timee / 4;   //4 core
    hour_ = timee / 3600;
    min_ = (timee - hour_ * 3600) / 60;
    sec_ = timee % 60;
    t = (char *)malloc(100);
    sprintf(t, "%d:%d:%d", hour_, min_, sec_);
    fclose(buf);
    return t;
}

char *get_cpu_name(char *str_1)
{
    FILE *buf;
    int i = 0;
    char *string = str_1;
    buf = fopen("/proc/cpuinfo", "r");
    for (i = 0; i < 5; i++)
    {
        fgets(string, 256, buf);
    }
    for (i = 0; i < 256; i++)
    {
        if (string[i] == ':')
        {
            break;
        }
    }
    i += 2;
    string += i;
    string[31] = '\0';
    fclose(buf);
    return string;
}

char *get_cpu_MHz(char *str_1)
{
    FILE *buf;
    int i = 0;
    char *string = str_1;
    buf = fopen("/proc/cpuinfo", "r");
    for (i = 0; i < 8; i++)
    {
        fgets(string, 256, buf);
    }
    for (i = 0; i < 256; i++)
    {
        if (string[i] == ':')
        {
            break;
        }
    }
    i += 2;
    string += i;
    string[10] = '\0';
    fclose(buf);
    return string;
}

char *get_cpu_cache(char *str_1)
{
    FILE *buf;
    int i = 0;
    char *string = str_1;
    buf = fopen("/proc/cpuinfo", "r");
    for (i = 0; i < 9; i++)
    {
        fgets(string, 256, buf);
    }
    for (i = 0; i < 256; i++)
    {
        if (string[i] == ':')
        {
            break;
        }
    }
    i += 2;
    string += i;
    string[9] = '\0';
    fclose(buf);
    return string;
}

char *system_version(char *str_1)
{
    FILE *buf;
    int i = 0;
    char *string = str_1;
    buf = fopen("/proc/version_signature", "r");
    if (buf == NULL)
    {
        perror("Open version file failed\n");
        exit(1);
    }
    fread(string, 1, 27, buf);
    string[27] = '\0';
    fclose(buf);
    return string;
}

void get_proc_info(GtkWidget *clist, int *p, int *q, int *r, int *s, int *t, int *x)
{
    DIR *dir;
    struct dirent *ptr;
    int i, j;
    FILE *fp;
    char buf[1024];
    char _buffer[1024];
    char *buffer = _buffer;
    char *buffer2;
    char proc_pid[1024];
    char proc_name[1024];
    char proc_stat[1024];
    char proc_pri[1024];
    char proc_mem[1024];
    char text[5][1024];
    gchar *txt[5];

    gtk_clist_set_column_width(GTK_CLIST(clist), 0, 70);
    gtk_clist_set_column_width(GTK_CLIST(clist), 1, 130);
    gtk_clist_set_column_width(GTK_CLIST(clist), 2, 80);
    gtk_clist_set_column_width(GTK_CLIST(clist), 3, 80);
    gtk_clist_set_column_width(GTK_CLIST(clist), 4, 70);
    gtk_clist_set_column_title(GTK_CLIST(clist), 0, "PID");
    gtk_clist_set_column_title(GTK_CLIST(clist), 1, "Name");
    gtk_clist_set_column_title(GTK_CLIST(clist), 2, "Status");
    gtk_clist_set_column_title(GTK_CLIST(clist), 3, "Priority");
    gtk_clist_set_column_title(GTK_CLIST(clist), 4, "Memory");

    gtk_clist_column_titles_show(GTK_CLIST(clist));

    dir = opendir("/proc");
    if (dir == NULL )
    {
        perror("open dir failed");
        exit(1);
    }
    while (ptr = readdir(dir))
    {
        if ((ptr->d_name)[0] >= 48 && (ptr->d_name)[0] <= 57)
        {
            (*p)++;
            sprintf(buf, "/proc/%s/stat", ptr->d_name);

            fp = fopen(buf, "r");
            if (fp == NULL)
            {
                perror("open stat failed.");
                continue;
            }
            fgets(buffer, 1024, fp);
            fclose(fp);

            for (i = 0; i < 1024; i++)
            {
                if (buffer[i] == ' ') break;
            }
            buffer[i] = '\0';
            strcpy(proc_pid, buffer);
            i += 2;
            buffer += i;
            for (i = 0; i < 1024; i++)
            {
                if (buffer[i] == ')') break;
            }
            buffer[i] = '\0';
            strcpy(proc_name, buffer);
            i += 2;
            buffer2 = buffer + i;
            buffer2[1] = '\0';
            strcpy(proc_stat, buffer2);
            for (i = 0, j = 0; i < 1024 && j < 15; i++)
            {
                if (buffer2[i] == ' ') j++;
            }
            buffer2 += i;
            for (i = 0; i < 1024; i++)
            {
                if (buffer2[i] == ' ') break;
            }
            buffer2[i] = '\0';
            strcpy(proc_pri, buffer2);
            for (j = 0; i < 1024 && j < 4; i++)
            {
                if (buffer2[i] == ' ') j++;
            }
            buffer2 += i;
            for (i = 0; i < 1024; i++)
            {
                if (buffer2[i] == ' ') break;
            }
            buffer2[i] = '\0';
            strcpy(proc_mem, buffer2);

            if (!strcmp(proc_stat, "R")) (*q)++;
            if (!strcmp(proc_stat, "S")) (*r)++;
            if (!strcmp(proc_stat, "Z")) (*s)++;
            if (!strcmp(proc_stat, "T")) (*t)++;
            if (!strcmp(proc_stat, "X")) (*x)++;

            sprintf(text[0], "%s", proc_pid);
            sprintf(text[1], "%s", proc_name);
            sprintf(text[2], "%s", proc_stat);
            sprintf(text[3], "%s", proc_pri);
            sprintf(text[4], "%s", proc_mem);

            txt[0] = text[0];
            txt[1] = text[1];
            txt[2] = text[2];
            txt[3] = text[3];
            txt[4] = text[4];

            gtk_clist_append(GTK_CLIST(clist), txt);
        }
    }
    closedir(dir);
}

void select_row_callback(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
    gtk_clist_get_text(GTK_CLIST(clist), row, column, &pid1);
    printf("%s\n", pid1);
}

char* mem_info()
{
    char buffer[100 + 1];
    char data[20];
    long all = 0, _free = 0;
    int i = 0, j = 0, count = 0;
    int fd;
    fd = open("/proc/meminfo", O_RDONLY);
    read(fd, buffer, 100);
    for (i = 0, j = 0; i < 100; i++, j++)
    {
        if (buffer[i] == ':')
            count++;
        if (buffer[i] == ':' && count == 1)
        {
            while (buffer[++i] == ' ');
            for (j = 0; j < 20; j++, i++)
            {
                if (buffer[i] == 'k')  break;
                data[j] = buffer[i];
            }
            data[--j] = '\0';
            all = atol(data) / 1024;
        }
        if (buffer[i] == ':' && count == 2)
        {
            while (buffer[++i] == ' ');
            for (j = 0; j < 20; j++, i++)
            {
                if (buffer[i] == 'k')  break;
                data[j] = buffer[i];
            }
            data[--j] = '\0';
            _free += atol(data) / 1024;
        }
    }
    mem_size = all;
    mem_free = _free;
    sprintf(mem_ratio, "Memory: %ldM/%ldM", (all - _free), all);
    close(fd);
    return mem_ratio;
}

char* cpu_info()
{
    long  user_, nice_, system_, idle_, total_;
    long total_c, idle_c;
    char cpu_t[10], buffer[70 + 1];
    int fd;
    fd = open("/proc/stat", O_RDONLY);
    read(fd, buffer, 70);
    sscanf(buffer, "%s %ld %ld %ld %ld", cpu_t, &user_, &nice_, &system_, &idle_);
    if (flag1 == 0)
    {
        flag1 = 1;
        idle = idle_;
        total = user_ + nice_ + system_ + idle_;
        cpu_used_ratio = 0;
    }
    else
    {
        total_ = user_ + nice_ + system_ + idle_;
        total_c = total_ - total;
        idle_c = idle_ - idle;
        cpu_used_ratio = 100 * (total_c - idle_c) / total_c;
        total = total_;
        idle = idle_;
    }
    close(fd);
    sprintf(cpu_ratio, "CPU：%0.1f%%", cpu_used_ratio);
    return cpu_ratio;
}

gint refresh_mem(gpointer mem_)
{
    gtk_label_set_text(GTK_LABEL(mem_), mem_info());
    gtk_widget_show(mem_);
    return TRUE;
}

gint refresh_cpu(gpointer cpu_)
{
    gtk_label_set_text(GTK_LABEL(cpu_), cpu_info());
    gtk_widget_show(cpu_);
    return TRUE;
}

gint refresh_time(gpointer time_)
{
    long times;
    struct tm *qtime;
    char t[100];
    times = time(NULL);
    qtime = localtime(&times);
    strftime(t, sizeof(t), "%Y-%m-%d %H:%M:%S", qtime);
    gtk_label_set_text(GTK_LABEL(time_), t);
    gtk_widget_show(time_);
    return TRUE;
}

void Processor(GtkWidget *widget, gpointer data)
{
    GtkWidget *window;
    GtkWidget *frame1;
    GtkWidget *label;
    char bufferf1[1000];
    char buf1[256], buf2[256], buf3[256];
    GtkWidget *vbox;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Processor");
    gtk_widget_set_size_request (window, 400, 150);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    frame1 = gtk_frame_new (" ");
    gtk_container_set_border_width (GTK_CONTAINER (frame1), 10);
    gtk_widget_set_size_request (frame1, 400, 130);
    sprintf(bufferf1, "CPU name：%s\n\nCPU MHz：%s\nCache：%s\n", get_cpu_name(buf1), get_cpu_MHz(buf2), get_cpu_cache(buf3));
    label = gtk_label_new (bufferf1);
    gtk_container_add (GTK_CONTAINER (frame1), label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_show (label);
    gtk_box_pack_start(GTK_BOX(vbox), frame1, FALSE, FALSE, 0);
    gtk_widget_show (frame1);

    gtk_widget_show (vbox);

    gtk_widget_show_all(window);
    gtk_main ();
}

void System(GtkWidget *widget, gpointer data)
{
    char* tim;
    GtkWidget *window;
    GtkWidget *frame2;
    GtkWidget *label;
    char bufferf2[1000];
    char buf1[256], buf2[256], buf3[256];
    GtkWidget *vbox;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Operating System");
    gtk_widget_set_size_request (window, 400, 150);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    frame2 = gtk_frame_new (" ");
    gtk_container_set_border_width (GTK_CONTAINER (frame2), 10);
    gtk_widget_set_size_request (frame2, 400, 130);
    tim = get_time1(buf2);
    sprintf(bufferf2, "OS：%s\n\nAll time   ：%s\n\nFree time：%s\n", system_version(buf1), tim, get_time2(buf3));
    free(tim);
    label = gtk_label_new (bufferf2);
    gtk_container_add (GTK_CONTAINER (frame2), label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_show (label);
    gtk_box_pack_start(GTK_BOX(vbox), frame2, FALSE, FALSE, 0);
    gtk_widget_show (frame2);

    gtk_widget_show (vbox);

    gtk_widget_show_all(window);
    gtk_main ();
}

void About(GtkWidget *widget, gpointer data)
{
    GtkWidget *window;
    GtkWidget *frame3;
    GtkWidget *label;
    char bufferf3[1000];
    GtkWidget *vbox;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "About");
    gtk_widget_set_size_request (window, 400, 150);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    frame3 = gtk_frame_new ("");
    gtk_container_set_border_width (GTK_CONTAINER (frame3), 10);
    gtk_widget_set_size_request (frame3, 400, 100);
    sprintf(bufferf3, "System Manager 1.0\t\n\nCopyright@2015 PYZ\t\t");
    label = gtk_label_new (bufferf3);
    gtk_container_add (GTK_CONTAINER (frame3), label);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_widget_show (label);
    gtk_box_pack_start(GTK_BOX(vbox), frame3, FALSE, FALSE, 0);
    gtk_widget_show (frame3);

    gtk_widget_show (vbox);

    gtk_widget_show_all(window);
    gtk_main ();
}

void cpu_draw(GtkWidget *widget)
{
    int i;
    int my_first_data;
    GdkColor color;
    GdkDrawable *wid_window;
    GdkGC *gc;
    wid_window = widget->window;
    gc = widget->style->fg_gc[GTK_WIDGET_STATE(widget)];

    gdk_draw_rectangle(wid_window, gc, TRUE, 10, 5, 650, 105);

    if (flag3 == 0)
    {
        for (i = 0; i < 215; i++)
            cpu_data[i] = 0;
        flag3 = 1;
        cpu_first_data = 0;
    }

    cpu_data[cpu_first_data] = cpu_used_ratio / 100;
    cpu_first_data++;
    if (cpu_first_data == 215) cpu_first_data = 0;

    color.red = 65535;
    color.green = 0;
    color.blue = 65535;
    gdk_gc_set_rgb_fg_color(gc, &color);

    my_first_data = cpu_first_data;
    for (i = 0; i < 214; i++)
    {
        gdk_draw_line(wid_window, gc, 10 + i * 3, 110 - 104 * cpu_data[my_first_data % 215], 10 + (i + 1) * 3, 110 - 104 * cpu_data[(my_first_data + 1) % 215]);
        my_first_data++;
        if (my_first_data == 215) my_first_data = 0;
    }

    color.red = 0;
    color.green = 0;
    color.blue = 0;
    gdk_gc_set_rgb_fg_color(gc, &color);
}

void mem_draw(GtkWidget *widget)
{
    int i;
    int my_first_data;
    GdkColor color;
    GdkDrawable *wid_window;
    GdkGC *gc;
    wid_window = widget->window;
    gc = widget->style->fg_gc[GTK_WIDGET_STATE(widget)];

    gdk_draw_rectangle(wid_window, gc, TRUE, 10, 5, 650, 105);

    if (flag3 == 0)
    {
        for (i = 0; i < 215; i++) mem_data[i] = 0;
        flag3 = 1;
        mem_first_data = 0;
    }

    mem_data[mem_first_data] = (float)(mem_size - mem_free) / mem_size;
    mem_first_data++;
    if (mem_first_data == 215) mem_first_data = 0;

    color.red = 65535;
    color.green = 65535;
    color.blue = 0;
    gdk_gc_set_rgb_fg_color(gc, &color);

    my_first_data = mem_first_data;
    for (i = 0; i < 214; i++)
    {
        gdk_draw_line(wid_window, gc, 10 + i * 3, 110 - 104 * mem_data[my_first_data % 215], 10 + (i + 1) * 3, 110 - 104 * mem_data[(my_first_data + 1) % 215]);
        my_first_data++;
        if (my_first_data == 215) my_first_data = 0;
    }

    color.red = 0;
    color.green = 0;
    color.blue = 0;
    gdk_gc_set_rgb_fg_color(gc, &color);
}

void End_Proc(GtkButton *button, gpointer user_data)
{
    char order[20];
    sprintf(order, "kill -9 %s", pid1);
    system(order);
}


int main(int argc, char *argv[])
{
    char buffer1[100], buffer2[100], buffer3[100];
    char bufferf1[1000];
    int p = 0, q = 0, r = 0, s = 0, d = 0, t = 0, x = 0;

    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *button1;
    GtkWidget *button2;
    GtkWidget *button3;
    GtkWidget *table;
    GtkWidget *note;
    GtkWidget *frame4;
    GtkWidget *frame5;
    GtkWidget *vbox;
    GtkWidget *vbox1;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *clist;
    GtkWidget *scrolled_window;
    GtkWidget *cpu_hbox;        //容纳cpu两个图
    GtkWidget *mem_hbox;        //容纳mem两个图
    GtkWidget *cpu_;            //cpu使用率
    GtkWidget *mem_;            //内存使用情况
    GtkWidget *time_;
    GtkWidget *cpu_record;      //cpu曲线图
    GtkWidget *mem_record;      //内存曲线图

    gtk_init(&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);      //create the main window
    gtk_window_set_title (GTK_WINDOW (window), "System Manager");
    gtk_widget_set_size_request (window, 720, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

    table = gtk_table_new (2, 6, FALSE);     //create a note
    gtk_container_add (GTK_CONTAINER (window), table);
    note = gtk_notebook_new ();
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (note), GTK_POS_TOP);
    gtk_table_attach_defaults (GTK_TABLE (table), note, 0, 6, 0, 1);
    gtk_widget_show (note);

//Note 1
    vbox = gtk_vbox_new(TRUE, 0);
    sprintf(buffer1, "Details");
    label = gtk_label_new (buffer1);
    gtk_notebook_append_page (GTK_NOTEBOOK (note), vbox, label);

    button = gtk_button_new_with_label("Processor");
    g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (Processor), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 10);

    button2 = gtk_button_new_with_label("System");
    g_signal_connect(G_OBJECT (button2), "clicked", G_CALLBACK (System), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button2, TRUE, TRUE, 10);

    button3 = gtk_button_new_with_label("About");
    g_signal_connect(G_OBJECT (button3), "clicked", G_CALLBACK (About), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button3, TRUE, TRUE, 10);

    gtk_widget_show (vbox);

//Note 2
    hbox = gtk_hbox_new(FALSE, 5);
    sprintf(buffer2, "Processes");
    label = gtk_label_new (buffer2);
    gtk_notebook_append_page (GTK_NOTEBOOK (note), hbox, label);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled_window, 500, 250);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    clist = gtk_clist_new(5);

    get_proc_info(clist, &p, &q, &r, &s, &t, &x);
    g_signal_connect(G_OBJECT(clist), "select_row", G_CALLBACK(select_row_callback), NULL);
    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), clist);
    gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, FALSE, FALSE, 5);

    vbox1 = gtk_vbox_new(FALSE, 5);
    frame4 = gtk_frame_new ("PROCESSES");
    gtk_widget_set_size_request (frame4, 100, 215);
    sprintf(bufferf1, "All process：%d\n\nRunning：%d\n\nSleeping：%d\n\nZombied：%d\n\nStopped:%d\n\nDead:%d\n\n", p, q, r, s, t, x);
    label = gtk_label_new (bufferf1);
    gtk_container_add (GTK_CONTAINER (frame4), label);
    gtk_box_pack_start(GTK_BOX(vbox1), frame4, FALSE, FALSE, 10);
    button1 = gtk_button_new_with_label("End Process");
    g_signal_connect(G_OBJECT (button1), "clicked", G_CALLBACK (End_Proc), NULL);
    gtk_box_pack_start(GTK_BOX(vbox1), button1, FALSE, FALSE, 10);

    gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 5);

    gtk_widget_show_all(hbox);

//Note 3
    vbox = gtk_vbox_new(FALSE, 10);
    sprintf(buffer3, "Resources");
    label = gtk_label_new(buffer3);
    gtk_notebook_append_page (GTK_NOTEBOOK (note), vbox, label);

    cpu_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), cpu_hbox, TRUE, TRUE, 2);
    frame5 = gtk_frame_new("CPU History");
    gtk_container_set_border_width (GTK_CONTAINER (frame5), 10);
    gtk_widget_set_size_request(frame5, 700, 150);
    gtk_box_pack_start(GTK_BOX(cpu_hbox), frame5, FALSE, FALSE, 0);
    cpu_drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_size_request(cpu_drawing_area, 700, 130);
    gtk_timeout_add(300, (GtkFunction)cpu_draw, (gpointer)cpu_drawing_area);
    gtk_container_add (GTK_CONTAINER(frame5), cpu_drawing_area);
    gtk_widget_show (cpu_drawing_area);
    gtk_widget_show(cpu_hbox);

    mem_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), mem_hbox, TRUE, TRUE, 2);
    frame5 = gtk_frame_new("Memory History");
    gtk_container_set_border_width (GTK_CONTAINER (frame5), 10);
    gtk_widget_set_size_request(frame5, 700, 150);
    gtk_box_pack_start(GTK_BOX(mem_hbox), frame5, FALSE, FALSE, 0);
    mem_drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_size_request(mem_drawing_area, 700, 130);
    gtk_timeout_add(300, (GtkFunction)mem_draw, (gpointer)mem_drawing_area);
    gtk_container_add (GTK_CONTAINER(frame5), mem_drawing_area);
    gtk_widget_show (mem_drawing_area);
    gtk_widget_show(mem_hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 2);
    gtk_widget_show(hbox);
    cpu_ = gtk_label_new("");
    mem_ = gtk_label_new("");
    gtk_timeout_add(500, (GtkFunction)refresh_cpu, (gpointer)cpu_);
    gtk_timeout_add(500, (GtkFunction)refresh_mem, (gpointer)mem_);
    gtk_box_pack_start(GTK_BOX(hbox), cpu_, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), mem_, TRUE, TRUE, 10);
    gtk_widget_show(mem_);
    gtk_widget_show(cpu_);

    time_ = gtk_label_new("");
    gtk_timeout_add(1000, (GtkFunction)refresh_time, (gpointer)time_);
    gtk_box_pack_start(GTK_BOX(hbox), time_, TRUE, TRUE, 10);
    gtk_widget_show(time_);

    button2 = gtk_button_new_with_label("Reboot");
    g_signal_connect(G_OBJECT (button2), "clicked", G_CALLBACK (reboot_computer), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button2, TRUE, TRUE, 10);


    gtk_widget_show(table);
    gtk_widget_show_all(window);
    gtk_main ();
    return 0;
}
