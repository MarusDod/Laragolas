#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <gtk-3.0/gtk/gtk.h>
#include <glib/gprintf.h>


#define ROOT_PATH "/var/www"
#define MYSQL_PATH "/var/lib/mysql"
#define MYSQL_PORT 3306
#define SERVER_PORT 8000
#define HOST_NAME "localhost"
#define APACHE_ETC "/etc/apache2/sites-enabled/"
#define HOST_ETC "/etc/hosts"

#ifdef RELEASE
    #define CONFIG_PATH "/etc/laragolas/"
#else 
    #define CONFIG_PATH "./"
#endif

static const char* virtual_host_template = 
    "<VirtualHost *:%d>\n"
    "   DocumentRoot \"%2$s/public/\"\n"
    "   ServerName %3$s.test\n"
    "   ServerAlias *.%3$s.test\n"
    "   <Directory \"%2$s/public/\">\n"
    "       AllowOverride All\n"
    "       Require all granted\n"
    "   </Directory>\n"
    "</VirtualHost>\n";
    

#define waitProcess(pid,str)\
    do{\
        int wstatus;\
        int w = waitpid(pid, &wstatus, WUNTRACED);\
        if (w == -1) {\
            perror(str);\
            exit(EXIT_FAILURE);\
        }\
    }while(0);

GtkBuilder* builder = NULL;
GtkWidget* window = NULL;
GtkWidget* settingsWindow = NULL;
GtkButton* startButton = NULL;
GtkMenu* menu = NULL;
GtkMenu* documentRootMenu = NULL;
GtkButton* webButton = NULL;
GtkButton* dbButton = NULL;
GtkButton* terminalButton = NULL;
GtkButton* rootButton = NULL;

GtkLabel* progressLabel = NULL;
GtkLabel* apacheStatus = NULL;
GtkLabel* mysqlStatus = NULL;

GtkButton* settingsButton = NULL;
GtkButton* reloadButton = NULL;

GtkEntry* dataDir = NULL;
GtkEntry* documentRoot = NULL;
GtkEntry* hostName = NULL;
GtkEntry* dbPort = NULL;
GtkEntry* serverPort = NULL;
GtkWidget* itemRoot = NULL;

GtkDialog* dialog = NULL;
GtkDialog* dialog1 = NULL;
GtkEntry* projectEntry = NULL;
GtkEntry* dbEntry = NULL;
GtkButton* cancelButton = NULL;
GtkButton* okButton = NULL;
GtkButton* cancelButton1 = NULL;
GtkButton* okButton1 = NULL;

FILE* config_file = NULL;

typedef struct {
    bool isrunning;
    char* root_path;
    char* mysql_path;
    uint16_t mysql_port;
    char* host;
    uint16_t port;
}Settings;

Settings settings = (Settings){
    .isrunning = false,
    .root_path = ROOT_PATH,
    .mysql_path = MYSQL_PATH,
    .mysql_port = MYSQL_PORT,
    .host = HOST_NAME,
    .port = SERVER_PORT
};

void exit_handler(void);
void write_virtual_hosts();
void change_hosts_entry(const char* string,bool insert);
void cleanup_hosts_entries();
void change_port_entry(int port);

int spawn_new_process(const char* str,bool asroot){
    pid_t pid = -1;

    switch(pid = fork()){
        case 0:
            if(!asroot){
                char* str2 = g_strescape(str,"\b\f\n\r\t\v");
                str2 = g_strescape(str2,"\b\f\n\r\t\v");
                char str3[500];
                snprintf(str3,500,"bash -c \"su $SUDO_USER -c \\\"%s\\\"\"",str2);
                system(str3);
            }
            else{
                system(str);
            }

            exit(0);
        case -1:
            exit(1);
        default:
            break;
    }

    return pid;
}

void message_box(const char* string) {
    GtkWidget* widget = gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"%s\n",string);
    gtk_dialog_run(GTK_DIALOG(widget));
}

//todo adicionar apache
gboolean check_mysql_service(){
    return system("systemctl is-active --quiet mysql.service") == 0;
}

gboolean check_apache_service(){
    return system("systemctl is-active --quiet apache2.service") == 0;
}

void set_documentRoot_popup();
void update_config_file();

void start_button_callback(GtkButton* button,gpointer data){
    int ret,pid;
    char holder[50];

    if(!settings.isrunning){
        pid = spawn_new_process("systemctl start mysql.service",true);

        waitProcess(pid,"");
        ret = check_mysql_service();

        if(!ret) 
            return;

        set_documentRoot_popup();

        pid = spawn_new_process("systemctl start apache2.service",true);

        waitProcess(pid,"");
        ret = check_apache_service();

        if(!ret) 
            return;

        
        sprintf(holder,"apache: %hu",settings.port);
        gtk_label_set_label(apacheStatus,holder);

        sprintf(holder,"mysql: %hu",settings.mysql_port);
        gtk_label_set_label(mysqlStatus,holder);

        gtk_button_set_label(button,"stop");
        gtk_widget_set_visible(GTK_WIDGET(reloadButton),true);
        gtk_widget_set_visible(GTK_WIDGET(itemRoot),true);
        settings.isrunning = true;

    }
    else{
        pid = spawn_new_process("systemctl stop mysql.service",true);
        waitProcess(pid,"");
        ret = check_mysql_service();

        if(ret) 
            return;

        pid = spawn_new_process("systemctl stop apache2.service",true);
        waitProcess(pid,"");
        ret = check_apache_service();

        if(ret) 
            return;

        gtk_button_set_label(button,"start");
        gtk_widget_set_visible(GTK_WIDGET(reloadButton),false);
        gtk_widget_set_visible(GTK_WIDGET(itemRoot),false);
        settings.isrunning = false;
        gtk_label_set_label(mysqlStatus,"");
        gtk_label_set_label(apacheStatus,"");
    }
}

void web_button_callback(GtkButton* button,gpointer data){
    char str[800];

    sprintf(str,"bash -c \"$(xdg-settings get default-web-browser | cut -f1 -d.) 'file://%s'\"",settings.root_path);
    spawn_new_process(str,false);
}

void reload_button_callback(GtkButton* button,gpointer data){
    if(!settings.isrunning) return;

    spawn_new_process("systemctl restart mysql.service",true);
    spawn_new_process("systemctl restart apache2.service",true);

    cleanup_hosts_entries();
    change_port_entry(settings.port);
    write_virtual_hosts();
}

void terminal_button_callback(GtkButton* button,gpointer data){
    char str [1000];
    sprintf(str,"cd %s && x-terminal-emulator",settings.root_path);
    spawn_new_process(str,false);
}

void db_button_callback(GtkButton* button,gpointer data){
    spawn_new_process("mysql-workbench",false);
}

void root_button_callback(GtkButton* button,gpointer data){
    char str[1000];
    sprintf(str,"xdg-open %s",settings.root_path);
    spawn_new_process(str,false);

}

void open_in_browser_button_callback(GtkMenuItem* item,gpointer data){
    char str[1000];
    const char* folder = gtk_menu_item_get_label(item);

    int port = settings.port;

    sprintf(str,"xdg-open http://%s.test:%d\n",folder,port);

    spawn_new_process(str,false);

}

void settings_button_callback(GtkButton* button,gpointer data){
    gtk_widget_show(settingsWindow);
}

void dataDir_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = gtk_entry_get_text(entry);

    settings.mysql_path = (char*) txt;
    update_config_file();

    reload_button_callback(NULL,NULL);
}

void documentRoot_changed_callback(GtkEntry* entry,gpointer data){
    settings.root_path = (char*)gtk_entry_get_text(entry);
    update_config_file();

    gtk_menu_item_set_label(GTK_MENU_ITEM(itemRoot),settings.root_path);
    set_documentRoot_popup();
    reload_button_callback(NULL,NULL);
}

void hostName_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = (char*)gtk_entry_get_text(entry);

    settings.host = (char*) txt;
    update_config_file();
    reload_button_callback(NULL,NULL);
}

void dbPort_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = (char*)gtk_entry_get_text(entry);

    settings.mysql_port = atoi(txt);
    update_config_file();
    reload_button_callback(NULL,NULL);
}

void serverPort_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = (char*) gtk_entry_get_text(entry);

    settings.port = atoi(txt);

    update_config_file();

    reload_button_callback(NULL,NULL);
}

void set_documentRoot_popup(){
    struct dirent *dp;
    DIR *dfd;
    char *dir = (char*) gtk_menu_item_get_label(GTK_MENU_ITEM(itemRoot));
    char filename_qfd[300] ;
    char str4[400];

    if ((dfd = opendir(dir)) == NULL){
        return;
    }

    if(documentRootMenu){
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemRoot),NULL);
    }
    documentRootMenu = GTK_MENU(gtk_menu_new());

    while ((dp = readdir(dfd)) != NULL){
        struct stat stbuf;

        if(!strcmp(".",dp->d_name) || !strcmp("..",dp->d_name)) continue;

        sprintf( filename_qfd , "%s/%s",dir,dp->d_name) ;
        sprintf(str4,"%s/public",filename_qfd);


        if(stat(str4,&stbuf) == -1 || (stbuf.st_mode & S_IFMT) != S_IFDIR)
            continue;


        GtkWidget* item = gtk_menu_item_new_with_label(dp->d_name);

        gtk_menu_shell_append(GTK_MENU_SHELL(documentRootMenu), item);
        gtk_widget_show(item);
        g_signal_connect(item, "activate",G_CALLBACK(open_in_browser_button_callback),strdup(filename_qfd));
    }

    closedir(dfd);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemRoot),GTK_WIDGET(documentRootMenu));
    gtk_widget_set_visible(GTK_WIDGET(itemRoot),settings.isrunning);

    cleanup_hosts_entries();
    write_virtual_hosts();
    change_port_entry(settings.port);
}

int show_popup(GtkWidget *widget, GdkEvent *event) {
  
  const gint RIGHT_CLICK = 3;
    
  if (event->type == GDK_BUTTON_PRESS && event->button.button == RIGHT_CLICK) {

    gtk_menu_popup_at_pointer(GTK_MENU(widget),event);
    return TRUE;
  }

  return FALSE;
}

int new_laravel_callback(GtkMenuItem* item,gpointer data){
    int response = gtk_dialog_run(dialog);

    switch(response){
        case GTK_RESPONSE_CANCEL:
            break;
        case GTK_RESPONSE_OK:
        {
            char n[300];
            sprintf(n,"composer create-project --prefer-dist laravel/laravel %s/%s",settings.root_path,gtk_entry_get_text(projectEntry));
            spawn_new_process(n,false);

        }
            break;
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    gtk_entry_set_text(projectEntry,"");

    return true;
}

int new_db_callback(GtkMenuItem* item,gpointer data){
    int response = gtk_dialog_run(dialog1);

    switch(response){
        case GTK_RESPONSE_CANCEL:
            break;
        case GTK_RESPONSE_OK:
        {
            char n[300];

            sprintf(n,"mysql -u root -e \"create database %s\"",gtk_entry_get_text(dbEntry));
            spawn_new_process(n,true);

        }
            break;
    }

    gtk_widget_destroy(GTK_WIDGET(dialog1));
    gtk_entry_set_text(dbEntry,"");

    return true;
}

int okButton_callback(GtkButton* button,gpointer data){
    gtk_dialog_response(dialog,GTK_RESPONSE_OK);

    return true;
}

int cancelButton_callback(GtkButton* button,gpointer data){
    gtk_dialog_response(dialog,GTK_RESPONSE_CANCEL);

    return true;
}

int okButton1_callback(GtkButton* button,gpointer data){
    gtk_dialog_response(dialog1,GTK_RESPONSE_OK);

    return true;
}

int cancelButton1_callback(GtkButton* button,gpointer data){
    gtk_dialog_response(dialog1,GTK_RESPONSE_CANCEL);

    return true;
}

int parse_config(char* filename){
    config_file = fopen(filename,"rw");

    settings.host = malloc(50);
    settings.mysql_path = malloc(100);
    settings.root_path = malloc(300);

    fscanf(config_file,"host:");
    fscanf(config_file,"%s",settings.host);

    fscanf(config_file," mysql-path:");
    fscanf(config_file,"%s",settings.mysql_path);

    fscanf(config_file," mysql-port:");
    fscanf(config_file,"%hd",&settings.mysql_port);

    fscanf(config_file," root-path:");
    fscanf(config_file,"%s",settings.root_path);

    fscanf(config_file," port:");
    fscanf(config_file,"%hd",&settings.port);

    switch(check_mysql_service() && check_apache_service()){
        case true:
            settings.isrunning = true;
            break;
        case 0:
            settings.isrunning = false;
            break;
    }

    return true;
}

void write_virtual_hosts(){
    char str[300];
    char str2[300];
    char str3[100];
    GList* children = gtk_container_get_children(GTK_CONTAINER(documentRootMenu));

    struct stat st;
    if ((stat (APACHE_ETC, &st)) == -1)
    {
        message_box ("no " APACHE_ETC " :(");
        exit(1);
    }

    for(;children;children = children->next){
        GtkMenuItem* item = children->data;

        const char* vHost = gtk_menu_item_get_label(item);

        sprintf(str,"%sauto.%s.conf",APACHE_ETC, vHost);
        sprintf(str2,"%s%s",settings.root_path, vHost);
        sprintf(str3,"%s.test",vHost);

        FILE* fptr = fopen(str,"w");

        if(!fptr){
            fprintf(stderr,"could not write to file %s,%s",str,strerror(errno));
            exit(1);
        }

        fprintf(fptr,virtual_host_template,settings.port,str2,vHost);
        change_hosts_entry(str3,true);
        fclose(fptr);
    }

}

void change_port_entry(int port){
    char str[300];
    static const char* path = "/etc/apache2/ports.conf";

    snprintf(str,300,"sudo bash "CONFIG_PATH"add_entry.sh port %d %s",port,path);
    system(str);
}

void change_hosts_entry(const char* string,bool insert){
    static const char* ip = "127.0.0.1";

    char str[300];
    snprintf(str,200,"bash " CONFIG_PATH"add_entry.sh %s %s %s %s",insert ? "add" : "delete",ip,string,HOST_ETC);
    system(str);
}

void cleanup_hosts_entries(){
    system("sudo sed -i \"/\\#laragolas\\!\\#/d\" " HOST_ETC);
    system("sudo rm -rf " APACHE_ETC "auto.*");
}

void update_config_file(){
    freopen(NULL,"w",config_file);

    fprintf(config_file,
        "host: %s\n"
        "mysql-path: %s\n"
        "mysql-port: %hd\n"
        "root-path: %s\n"
        "port: %d\n",
        settings.host,settings.mysql_path,settings.mysql_port,settings.root_path,settings.port);

    fflush(config_file);
}



int main(int argc,char* argv[]){
    parse_config(CONFIG_PATH"config.txt");

    gtk_init(&argc,&argv);
    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder,CONFIG_PATH"gui.glade",NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder,"mainWindow"));
    settingsWindow = GTK_WIDGET(gtk_builder_get_object(builder,"settingsWindow"));
    dialog = GTK_DIALOG(gtk_builder_get_object(builder,"laravelDialog"));
    dialog1 = GTK_DIALOG(gtk_builder_get_object(builder,"newdbDialog"));

    menu = GTK_MENU(gtk_menu_new());


    GtkWidget* item1 = gtk_menu_item_new_with_label("new laravel project");
    g_signal_connect(G_OBJECT(item1), "activate", G_CALLBACK (new_laravel_callback), NULL);

    GtkWidget* item2 = gtk_menu_item_new_with_label("new database");
    g_signal_connect(G_OBJECT(item2), "activate", G_CALLBACK (new_db_callback), NULL);



    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item1);
    gtk_widget_show(item1);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item2);
    gtk_widget_show(item2);


    itemRoot = gtk_menu_item_new_with_label(settings.root_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemRoot);
    gtk_widget_show(itemRoot);

    g_signal_connect_swapped(G_OBJECT(window), "button-press-event",G_CALLBACK(show_popup), menu);
    g_signal_connect(G_OBJECT(window),"destroy", gtk_main_quit, NULL);
    g_signal_connect (settingsWindow, "delete_event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (G_OBJECT(dialog), "close", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (G_OBJECT(dialog1), "close", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    startButton = GTK_BUTTON(gtk_builder_get_object(builder, "startButton"));
    webButton = GTK_BUTTON(gtk_builder_get_object(builder, "webButton"));
    dbButton =  GTK_BUTTON(gtk_builder_get_object(builder, "dbButton"));
    terminalButton = GTK_BUTTON(gtk_builder_get_object(builder, "terminalButton"));
    rootButton = GTK_BUTTON(gtk_builder_get_object(builder, "rootButton"));
    reloadButton = GTK_BUTTON(gtk_builder_get_object(builder, "reloadButton"));
    okButton = GTK_BUTTON(gtk_builder_get_object(builder, "okButton"));
    cancelButton = GTK_BUTTON(gtk_builder_get_object(builder, "cancelButton"));
    projectEntry = GTK_ENTRY(gtk_builder_get_object(builder, "projectEntry"));
    dbEntry = GTK_ENTRY(gtk_builder_get_object(builder, "dbEntry"));

    progressLabel = GTK_LABEL(gtk_builder_get_object(builder, "progressLabel"));
    apacheStatus = GTK_LABEL(gtk_builder_get_object(builder, "apacheStatus"));
    mysqlStatus = GTK_LABEL(gtk_builder_get_object(builder, "mysqlStatus"));

    okButton1 = GTK_BUTTON(gtk_builder_get_object(builder, "okButton1"));
    cancelButton1 = GTK_BUTTON(gtk_builder_get_object(builder, "cancelButton1"));

    settingsButton = GTK_BUTTON(gtk_builder_get_object(builder, "settingsButton"));

    documentRoot = GTK_ENTRY(gtk_builder_get_object(builder, "documentRootEntry"));
    gtk_entry_set_text(documentRoot,settings.root_path);
    dataDir = GTK_ENTRY(gtk_builder_get_object(builder, "dataDirEntry"));
    gtk_entry_set_text(dataDir,settings.mysql_path);

    hostName = GTK_ENTRY(gtk_builder_get_object(builder, "hostNameEntry"));
    gtk_entry_set_text(hostName,settings.host);


    char sport[20];
    sprintf(sport,"%d",settings.port);
    serverPort = GTK_ENTRY(gtk_builder_get_object(builder, "serverPort"));
    gtk_entry_set_text(serverPort,sport);

    char port[20];
    sprintf(port,"%d",settings.mysql_port);
    dbPort = GTK_ENTRY(gtk_builder_get_object(builder, "dbPortEntry"));
    gtk_entry_set_text(dbPort,port);

    g_signal_connect(GTK_WIDGET (startButton), "clicked",G_CALLBACK(start_button_callback),NULL);
    g_signal_connect(GTK_WIDGET (webButton), "clicked",G_CALLBACK(web_button_callback),NULL);
    g_signal_connect(GTK_WIDGET (dbButton), "clicked",G_CALLBACK(db_button_callback),NULL);
    g_signal_connect(GTK_WIDGET (terminalButton), "clicked",G_CALLBACK(terminal_button_callback),NULL);
    g_signal_connect(GTK_WIDGET (rootButton), "clicked",G_CALLBACK(root_button_callback),NULL);
    g_signal_connect(GTK_WIDGET (reloadButton), "clicked",G_CALLBACK(reload_button_callback),NULL);
    g_signal_connect(GTK_WIDGET (settingsButton), "clicked",G_CALLBACK(settings_button_callback),NULL);

    g_signal_connect(GTK_WIDGET (okButton), "clicked",G_CALLBACK(okButton_callback),NULL);
    g_signal_connect(GTK_WIDGET (cancelButton), "clicked",G_CALLBACK(cancelButton_callback),NULL);

    g_signal_connect(GTK_WIDGET (okButton1), "clicked",G_CALLBACK(okButton1_callback),NULL);
    g_signal_connect(GTK_WIDGET (cancelButton1), "clicked",G_CALLBACK(cancelButton1_callback),NULL);

    g_signal_connect(GTK_WIDGET (documentRoot), "activate",G_CALLBACK(documentRoot_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (dataDir), "activate",G_CALLBACK(dataDir_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (hostName), "activate",G_CALLBACK(hostName_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (dbPort), "activate",G_CALLBACK(dbPort_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (serverPort), "activate",G_CALLBACK(serverPort_changed_callback),NULL);

    settings.isrunning = !settings.isrunning;
    start_button_callback(startButton,NULL);

    gtk_widget_show(window);
    gtk_main();
    g_object_unref(builder);
    fclose(config_file);

    return 0;
}