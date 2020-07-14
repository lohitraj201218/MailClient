#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#define FROM_ADDR    "<>"
#define TO_ADDR      "<>"
#define CC_ADDR      "<>"

#define FROM    "Lohit Raj" FROM_ADDR
#define TO      "Giridhar Prasath" TO_ADDR
#define CC      "Ananthu S"CC_ADDR

#define ClientID "xxxxxxxx"
#define Secret   "xxxxxxxxx"

typedef struct a_token_s {
    char        *token;
    size_t    size;
}a_token_t;

typedef struct resp_data_s {
    char     *resp;
    size_t   size;
}resp_data_t;

struct upload_status {
  int lines_read;
};

static const char *payload_text[] = {
  "To: " TO "\r\n",
  "From: " FROM "\r\n",
  "Cc: " CC "\r\n",
  "Subject: SMTP TLS example message\r\n",
  "\r\n", /* empty line to divide headers from body, see RFC5322 */
  "The body of the message starts here.\r\n",
  "\r\n",
  "It could be a lot of lines, could be MIME encoded, whatever.\r\n",
  "Check RFC5322.\r\n",
  NULL
};

static void parse_token(resp_data_t     *rsp_data,
                        a_token_t       *a_token)
{
    char *tmp, *tmp2;
    char *needle = "\"access_token\":\"";
    char token[100];
    int len;

    tmp = strstr(rsp_data->resp, needle);
    if (tmp) {
        tmp = tmp + strlen(needle);
        a_token->token = malloc(2048);
        if (!a_token->token){
            return;
        }
        a_token->size = 2048;

        tmp2 = strstr(tmp, "\"}");
        if (!tmp2) {
            free(a_token->token);
            return;
        }
        len = tmp2 - tmp;
        if (len + 1 > a_token->size) {
            free(a_token->token);
            return;
        }
        snprintf(a_token->token, len + 1, "%s", tmp);
        a_token->token[len] = '\0';

        printf("FINAL:\n%s\n", a_token->token);
    }
    return;
}

static size_t cb(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t          realsize = size * nmemb;
    resp_data_t    *rsp_data = (resp_data_t *)userp;

    char *ptr = realloc(rsp_data->resp, rsp_data->size + realsize + 1);
    if(ptr == NULL)
      return 0;  /* out of memory! */

    rsp_data->resp = ptr;
    memcpy(&(rsp_data->resp[rsp_data->size]), data, realsize);
    rsp_data->size += realsize;
    rsp_data->resp[rsp_data->size] = 0;

    printf("%s\n", rsp_data->resp);
    return realsize;
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
    struct upload_status *upload_ctx = (struct upload_status *)userp;
    const char *data;

    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
      return 0;
    }

    data = payload_text[upload_ctx->lines_read];

    if(data) {
      size_t len = strlen(data);
      memcpy(ptr, data, len);
      upload_ctx->lines_read++;

      return len;
    }

    return 0;
}

int send_mail(a_token_t     *a_token){

    printf("SEND MAIL START\n");
    CURL *hnd;
    CURLcode ret = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct upload_status upload_ctx;
    char       username[2048 + 50] = "<>";
    char       *usr_pwd;

    upload_ctx.lines_read = 0;

    hnd = curl_easy_init();
    if (!hnd) {
      return 1;
    }  

    if (!strlen(a_token->token)){
        printf("no token\n");
        return 1;
    }

    usr_pwd = strcat(username, a_token->token);
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.68.0");
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(hnd, CURLOPT_SSH_KNOWNHOSTS, "/home/lohit/.ssh/known_hosts");
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERNAME, "<>");
    curl_easy_setopt(hnd, CURLOPT_PASSWORD, a_token->token);
    curl_easy_setopt(hnd, CURLOPT_USERPWD, usr_pwd);
    curl_easy_setopt(hnd, CURLOPT_HTTPAUTH, CURLAUTH_ANYSAFE);

    curl_easy_setopt(hnd, CURLOPT_XOAUTH2_BEARER, a_token->token);

    curl_easy_setopt(hnd, CURLOPT_URL, "smtp://smtp.office365.com:587");
    curl_easy_setopt(hnd, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

    curl_easy_setopt(hnd, CURLOPT_MAIL_FROM, FROM_ADDR);

    recipients = curl_slist_append(recipients, TO_ADDR);
    recipients = curl_slist_append(recipients, CC_ADDR);
    curl_easy_setopt(hnd, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(hnd, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(hnd, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(hnd, CURLOPT_UPLOAD, 1L);

    /* Send the message */
    ret = curl_easy_perform(hnd);
  
    /* Check for errors */
    if(ret != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(ret));

    /* Free the list of recipients */
    curl_slist_free_all(recipients);

        /* Always cleanup */
    curl_easy_cleanup(hnd);

    return (int)ret;
}

int main(int argc, char *argv[])
{
    CURLcode ret;
    CURL *hnd;

    char *url= "https://login.microsoftonline.com/oauth2/token";
  
    hnd = curl_easy_init();
    if (!hnd) {
      return 1;
    }
 
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(hnd, CURLOPT_URL, url);
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(hnd, CURLOPT_USERPWD, ClientID":"Secret);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, "grant_type=client_credentials&scope=User.Read,openid,profile,email,offline_access");
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)29);
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.68.0");
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(hnd, CURLOPT_SSH_KNOWNHOSTS, "/home/lohit/.ssh/known_hosts");
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_LOCALPORT, 2020L);
    curl_easy_setopt(hnd, CURLOPT_LOCALPORTRANGE, 1L);
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  
    resp_data_t rsp_data;
    rsp_data.resp = NULL;
    rsp_data.size = 0;

    curl_easy_setopt(hnd, CURLOPT_WRITEDATA,(void *)&rsp_data);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, cb);
  
    ret = curl_easy_perform(hnd);

    curl_easy_cleanup(hnd);
    hnd = NULL;

    a_token_t a_token;
    a_token.size = 0;
    parse_token(&rsp_data, &a_token);

    send_mail(&a_token);

    free(rsp_data.resp);
    free(a_token.token);
  
    return (int)ret;
}
/**** End of sample code ****/
