/* transfer.x - IDL file */

struct file_data {
    string filename<256>; /* Name of the file */
    opaque content<>;     /* Raw binary content of the file */
    int data_len;         /* Actual length of the data */
};

program FILE_TRANSFER_PROG {
    version FILE_TRANSFER_VERS {
        int UPLOAD_FILE(file_data) = 1;
    } = 1;
} = 0x31230000;