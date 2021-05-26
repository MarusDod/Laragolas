#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#include <gtk-3.0/gtk/gtk.h>


#define ROOT_PATH "/var/www"
#define MYSQL_PATH "/var/lib/mysql"
#define MYSQL_PORT 3306
#define SERVER_PORT 8000
#define HOST_NAME "localhost"

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

GtkButton* settingsButton = NULL;
GtkButton* reloadButton = NULL;

GtkEntry* dataDir = NULL;
GtkEntry* documentRoot = NULL;
GtkEntry* hostName = NULL;
GtkEntry* dbPort = NULL;
GtkEntry* serverPort = NULL;
GtkWidget* itemRoot = NULL;

GtkDialog* dialog = NULL;
GtkEntry* projectEntry = NULL;
GtkButton* cancelButton = NULL;
GtkButton* okButton = NULL;

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

int spawn_new_process(const char* str){
    pid_t pid = -1;

    switch(pid = fork()){
        case 0:
            system(str);
            exit(0);
        case -1:
            exit(1);
        default:
            break;
    }

    return pid;
}

void set_documentRoot_popup();
void update_config_file();

void start_button_callback(GtkButton* button,gpointer data){
    if(!settings.isrunning){
        spawn_new_process("systemctl start mysql.service");
        gtk_button_set_label(button,"stop");
        settings.isrunning = true;
    }
    else{
        spawn_new_process("systemctl stop mysql.service");
        gtk_button_set_label(button,"start");
        settings.isrunning = false;
    }
}

void web_button_callback(GtkButton* button,gpointer data){
    char str[300];

    sprintf(str,"firefox %s",settings.root_path);
    system(str);
}

void reload_button_callback(GtkButton* button,gpointer data){
    spawn_new_process("systemctl restart mysql.service");
}

void terminal_button_callback(GtkButton* button,gpointer data){
    char str [1000];
    sprintf(str,"cd %s && x-terminal-emulator",settings.root_path);
    spawn_new_process(str);
}

void db_button_callback(GtkButton* button,gpointer data){
    spawn_new_process("mysql-workbench");
}

void root_button_callback(GtkButton* button,gpointer data){
    char str[1000];
    sprintf(str,"xdg-open %s",settings.root_path);
    spawn_new_process(str);

}

void php_serve_button_callback(GtkMenuItem* item,gpointer data){
    char str[1000];
    char str2[1000];
    //char* filename = (char*)gtk_menu_item_get_label(item);

    sprintf(str,"php %s/artisan serve --host=%s --port=%d\n",(char*)data,settings.host,settings.port);
    g_print("%s",str);
    sprintf(str2,"xdg-open http:/%s:%d\n",settings.host,settings.port);
    g_print("%s",str2);

    spawn_new_process(str);

    sleep(2);
    spawn_new_process(str2);

    free(data);
}

void settings_button_callback(GtkButton* button,gpointer data){
    gtk_widget_show(settingsWindow);
}

void dataDir_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = gtk_entry_get_text(entry);

    settings.mysql_path = (char*) txt;
    update_config_file();
}

void documentRoot_changed_callback(GtkEntry* entry,gpointer data){
    settings.root_path = (char*)gtk_entry_get_text(entry);
    update_config_file();

    gtk_menu_item_set_label(GTK_MENU_ITEM(itemRoot),settings.root_path);
    set_documentRoot_popup();
}

void hostName_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = (char*)gtk_entry_get_text(entry);

    settings.host = (char*) txt;
    update_config_file();
}

void dbPort_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = (char*)gtk_entry_get_text(entry);

    settings.mysql_port = atoi(txt);
    update_config_file();
}

void serverPort_changed_callback(GtkEntry* entry,gpointer data){
    const char* txt = (char*) gtk_entry_get_text(entry);

    settings.port = atoi(txt);
    update_config_file();
}

void set_documentRoot_popup(){
    struct dirent *dp;
    DIR *dfd;
    char *dir = (char*) gtk_menu_item_get_label(GTK_MENU_ITEM(itemRoot));
    char filename_qfd[1000] ;

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
        stat(filename_qfd,&stbuf);

        if((stbuf.st_mode & S_IFMT ) == S_IFDIR ){
            GtkWidget* item = gtk_menu_item_new_with_label(dp->d_name);

            gtk_menu_shell_append(GTK_MENU_SHELL(documentRootMenu), item);
            gtk_widget_show(item);
            g_signal_connect(item, "activate",G_CALLBACK(php_serve_button_callback),strdup(filename_qfd));

        }
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(itemRoot),GTK_WIDGET(documentRootMenu));
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
            spawn_new_process(n);

        }
            break;
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    gtk_entry_set_text(projectEntry,"");

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

    //fprintf(stderr,"%s\n",settings.mysql_path);
    //fprintf(stderr,"%d\n",settings.mysql_port);
    //fprintf(stderr,"%s\n",settings.root_path);
    //fprintf(stderr,"%d\n",settings.port);
    //assert(!strcmp(settings.mysql_path,"/var/lib/mysql"));
    //assert(settings.mysql_port == 3306);
    //assert(!strcmp(settings.root_path,"/var/www"));
    //assert(settings.port == 8000);

    return true;
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
    parse_config("/home/marcos/Desktop/laragolas/config.txt");

    gtk_init(&argc,&argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder,"gui.glade",NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder,"mainWindow"));
    settingsWindow = GTK_WIDGET(gtk_builder_get_object(builder,"settingsWindow"));
    dialog = GTK_DIALOG(gtk_builder_get_object(builder,"laravelDialog"));

    menu = GTK_MENU(gtk_menu_new());


    GtkWidget* item1 = gtk_menu_item_new_with_label("new laravel project");
    g_signal_connect(G_OBJECT(item1), "activate", G_CALLBACK (new_laravel_callback), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item1);
    gtk_widget_show(item1);

    itemRoot = gtk_menu_item_new_with_label(settings.root_path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemRoot);
    gtk_widget_show(itemRoot);
    set_documentRoot_popup();

    g_signal_connect_swapped(G_OBJECT(window), "button-press-event",G_CALLBACK(show_popup), menu);
    g_signal_connect(G_OBJECT(window),"destroy", gtk_main_quit, NULL);
    g_signal_connect (settingsWindow, "delete_event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    g_signal_connect (G_OBJECT(dialog), "close", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

    startButton = GTK_BUTTON(gtk_builder_get_object(builder, "startButton"));
    webButton = GTK_BUTTON(gtk_builder_get_object(builder, "webButton"));
    dbButton =  GTK_BUTTON(gtk_builder_get_object(builder, "dbButton"));
    terminalButton = GTK_BUTTON(gtk_builder_get_object(builder, "terminalButton"));
    rootButton = GTK_BUTTON(gtk_builder_get_object(builder, "rootButton"));
    reloadButton = GTK_BUTTON(gtk_builder_get_object(builder, "reloadButton"));
    okButton = GTK_BUTTON(gtk_builder_get_object(builder, "okButton"));
    cancelButton = GTK_BUTTON(gtk_builder_get_object(builder, "cancelButton"));
    projectEntry = GTK_ENTRY(gtk_builder_get_object(builder, "projectEntry"));

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

    g_signal_connect(GTK_WIDGET (documentRoot), "activate",G_CALLBACK(documentRoot_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (dataDir), "activate",G_CALLBACK(dataDir_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (hostName), "activate",G_CALLBACK(hostName_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (dbPort), "activate",G_CALLBACK(dbPort_changed_callback),NULL);
    g_signal_connect(GTK_WIDGET (serverPort), "activate",G_CALLBACK(serverPort_changed_callback),NULL);


    gtk_widget_show(window);
    gtk_main();
    g_object_unref(builder);
    fclose(config_file);

    return 0;
}