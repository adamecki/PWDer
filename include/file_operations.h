void read_spkstate();
void save_spkstate(String state = "0");

void save_config(String input_mode = "2", String ssid = "sample", String wpwd = "password",
                 String ipaddr = "192.168.1.100", String port = "7305", String devpwd = "default",
                 String timeout = "5");
void init_sample_config();
void read_and_verify_config();

void init_sample_secret();
void read_and_verify_secret();

void file_password_import();
void export_vault();
