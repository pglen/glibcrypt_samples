
/* =====[ asdecrypt.c ]=========================================================

   Description:     Encryption excamples. Feasability study for diba
                    [Digital Bank].

   Revisions:

      REV   DATE            BY              DESCRIPTION
      ----  -----------     ----------      ------------------------------
      0.00  jul.14.2017     Peter Glen      Initial version.

   ======================================================================= */

#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "gcrypt.h"
#include "gcry.h"
#include "zmalloc.h"
#include "getpass.h"
#include "gsexp.h"

static int dump = 0;
static int verbose = 0;
static int test = 0;
static int ppub = 0;
static int nocrypt = 0;
static int use_stdin = 0;

static char    infile[MAX_PATH] = {'\0'};
static char    outfile[MAX_PATH] = {'\0'};
static char    keyfile[MAX_PATH] = {'\0'};
static char    thispass[MAX_PATH] = {'\0'};

char usestr[] = "asdecrypt [options] keyfile";

opts opts_data[] = {
                    'i',  "infile",  NULL, infile,  0, 0, NULL, 
                    "-i <filename>  --infile <filename>     - input file name",
                    
                    'o',  "outfile",  NULL, outfile,  0, 0, NULL, 
                    "-o <filename>  --outfile <filename>    - output file name",
                   
                    'p',   "pass",   NULL,  thispass, 0, 0,    NULL, 
                     "-p             --pass                 - pass in for key (testing only)",
                    
                    'k',  "keyfile",  NULL, keyfile,  0, 0, NULL, 
                    "-k <filename>  --keyfile <filename>    - key file name",

                    'r',  "stdin",    NULL, NULL,  0, 0, &use_stdin, 
                    "-r             --stdin                 - use stdin as input",
                    
                    'v',   "verbose",  NULL, NULL,  0, 0, &verbose, 
                    "-v             --verbose               - Verbosity on",
                    
                    'd',   "dump",  NULL, NULL,  0, 0, &dump, 
                    "-d             --dump                  - Dump buffers",
                    
                    't',   "test",  NULL,  NULL, 0, 0, &test, 
                    "-t             --test                  - test on",
                    
                    'n',   "nocrypt",  NULL,  NULL, 0, 0, &nocrypt, 
                    "-n             --nocrypt               - do not decypt private key",
                   
                    'x',   "printpub",  NULL,  NULL, 0, 0, &ppub, 
                    "-x             --printpub              - print public key",
                    
                     0,     NULL,  NULL,   NULL,   0, 0,  NULL, NULL,
                    };


static void myfunc(int sig)
{
    printf("\nSignal %d (segment violation)\n", sig);
    exit(111);
}

int main(int argc, char** argv)
{
    signal(SIGSEGV, myfunc);

    char *err_str;
    int nn = parse_commad_line(argv, opts_data, &err_str);
    if (err_str)
        {
        printf(err_str);
        usage(usestr, opts_data); exit(2);
        }

    if (argc - nn != 2 && strlen(keyfile) == 0) {
        printf("Missing argument for ascrypt.");
        usage(usestr, opts_data); exit(2);
    }

    //printf("thispass '%s'\n", thispass);
    
    //zverbose(TRUE);
    //gcry_set_allocation_handler(zalloc, NULL, NULL, zrealloc, zfree);
    
    gcrypt_init();
    gcry_error_t err;
    
    char* fname = argv[1 + nn];

    FILE* lockf = fopen(fname, "rb");
    if (!lockf) {
        xerr2("Opening of composite key failed on file '%s'.", fname);
    }
    
    /* Read and decrypt the key pair from disk. */
    unsigned int flen = getfsize(lockf);
    zline2(__LINE__, __FILE__);
    char* fbuf = zalloc(flen + 1);
    if (!fbuf) {
        xerr("malloc: could not allocate rsa buffer");
    }
    if (fread(fbuf, flen, 1, lockf) != 1) {
        xerr2("Reading of composite key failed on file '%s'.", fname);
    }
    fclose(lockf);
    
    //fbuf[flen] = '\0';
    zcheck(fbuf, __LINE__);
    
    zline2(__LINE__, __FILE__);
    int  rsa_len = flen;
    char *rsa_buf = decode_comp_key(fbuf, &rsa_len, &err_str);
    zfree(fbuf);
    
    //if(dump)
    //    dump_mem(rsa_buf, rsa_len);
    
    if (!rsa_buf) {
        //printf("%s\n", err_str);
        xerr2("Decode key failed. %s", err_str);
    }
    
    //if(dump)
    //  dump_mem(fbuf, flen);
    
    zline2(__LINE__, __FILE__);
    /* Grab a key pair password and create an AES context with it. */
    if(thispass[0] == '\0' && !nocrypt)
        {
        //getpass2(thispass, MAXPASSLEN, TRUE, TRUE);
        getpassx  passx;
        passx.prompt  = "Enter keypair pass:";
        passx.pass = thispass;    
        passx.maxlen = MAXPASSLEN;
        passx.minlen = 3;
        passx.weak   = TRUE;
        passx.nodouble = TRUE;
        passx.strength = 4;
        int ret = getpass2(&passx);
        if(ret < 0)
            xerr("Error on password entry.");
        }
        
    //printf("thispass '%s'\n", thispass);
    
    gcry_cipher_hd_t aes_hd;
    get_aes_ctx(&aes_hd, thispass, strlen(thispass));

    zline2(__LINE__, __FILE__);
    if(!nocrypt)
    {
        // Decrypt buffer
        err = gcry_cipher_decrypt(aes_hd, (unsigned char*) rsa_buf,
                                  rsa_len, NULL, 0);
        if (err) {
            xerr("gcrypt: failed to decrypt key pair");
        }
    }
    
    //if(dump)
    //    dump_mem(rsa_buf, rsa_len);
    
    // Extract our key
    //gcry_sexp_t glib_keys;
    //err = gcry_sexp_new(&glib_keys, rsa_buf, rsa_len, TRUE);
    //if(err)
    //    {
    //    xerr("Cannot decompose glib keys\n");
    //    }
        
    //gcry_sexp_t rsa_raw = gcry_sexp_find_token(glib_keys, "keydata", 0);
    //if(rsa_raw == NULL)
    //    {
    //    xerr("No key data here\n");
    //    }
    
    zline2(__LINE__, __FILE__);
    /* Load the key pair components into sexps. */
    gcry_sexp_t rsa_keypair;
    err = gcry_sexp_new(&rsa_keypair, rsa_buf, rsa_len, 0);
    if(err)
        {
        // Delay a little to fool DOS attacks
        struct timespec ts = {0, 300000000};
        nanosleep(&ts, NULL);
        xerr2("Failed to load composit key. (pass?)");
        }
        
    gcry_sexp_t privk = gcry_sexp_find_token(rsa_keypair, "private-key", 0);
    if(privk == NULL)
        {
        xerr2("No private key present in '%s'", fname);
        }
    //print_sexp(privk);
    
    zline2(__LINE__, __FILE__);
    int keylen =  gcry_pk_get_nbits(privk) / 8 ;
    //printf("Key length : %d\n", keylen);
    
    char *data_buf = NULL;
    unsigned int data_len;
    
    if(use_stdin)
        {
        //printf("Using stdin\n");
        zline2(__LINE__, __FILE__);
        char *fbuf2 = zalloc(20000);
        int  xidx = 0;
        while(TRUE)
            {
            char chh = getc(stdin);
            if(feof(stdin))
                {
                data_len = xidx;
                break;
                }
            fbuf2[xidx++] = chh;
            if(xidx > 10000)
                xerr("Out of preallocated stdin buffer\n");
            }
            
        //printf("Got stdin buffer %d byte\n%s\n", data_len, fbuf2);
            
        char *err_str;
        data_buf  = decode_rsa_cyph(fbuf2, &data_len, &err_str);
        if(!data_buf)
            {
            xerr2("Cannot decode input file. %s\n", err_str);
            }
        zfree(fbuf2); 
        }
    else if(infile[0] != '\0')
       {
        FILE* lockf2 = fopen(infile, "rb");
        if (!lockf2) {
            xerr2("Cannot open input file '%s'.", infile);
        }
        
        /* Read and decrypt the key pair from disk. */
        data_len = getfsize(lockf2);
        zline2(__LINE__, __FILE__);
        char* fbuf2 = zalloc(data_len + 1);
        if (!fbuf2) {
            xerr("Could not allocate plain text buffer");
        }
        if (fread(fbuf2, data_len, 1, lockf2) != 1) {
            xerr("Cannot reead input data.");
        }
        fclose(lockf2);
        //dump_mem(fbuf2, data_len);
        
        char *err_str;
        data_buf  = decode_rsa_cyph(fbuf2, &data_len, &err_str);
        if(!data_buf)
            {
            xerr2("Cannot decode input file. %s\n", err_str);
            }
        zfree(fbuf2);     
       }
    else
        {
        xerr("Need data to decrypt.\n");
        }
        
    //dump_mem(data_buf, data_len);
    int  outlen2 = 0;
    zline2(__LINE__, __FILE__);
    int limlen =  data_len * 2 + keylen;
    char *outptr = zalloc(limlen);
    if(!outptr)
        {
        xerr("Cannot allocate output memory.");
        }
    for(int loop = 0; loop < data_len; /* loop += keylen */)
        {
        short curr_len = *( (short*) (data_buf + loop) );
        loop += sizeof(short);
        //printf("data %p len = %d ", data_buf + loop, curr_len);
        
        if(loop + curr_len > data_len)
            {
            xerr2("Reading past last byte.");
            }
        
        /* Create a message. */
        gcry_sexp_t ciph;
        int err_offs;
        gcry_error_t err2 = gcry_sexp_build(&ciph, NULL, 
                                    "(enc-val (rsa (a %b)))", 
                                        curr_len, data_buf + loop);
        //print_sexp(ciph);
        
        /* Decrypt the message. */
        gcry_sexp_t plain;
        err = gcry_pk_decrypt(&plain, ciph, privk);
        if (err) {
            xerr("gcrypt: decryption failed");
            }
    
        /* Pretty-print the results. */
        gcry_mpi_t out_msg = gcry_sexp_nth_mpi(plain, 0, GCRYMPI_FMT_USG);
        
        int plen = 0; unsigned char *buffm;                                     
        err = gcry_mpi_aprint(GCRYMPI_FMT_USG, &buffm, &plen, out_msg);
        if (err) 
            {
            xerr("failed to stringify mpi");
            } 
        //printf("outlen %d\n", plen);
        
        if(outlen2 + plen > limlen)
            {
            xerr("Could not write all mem to buffer");
            } 
        memcpy(outptr + outlen2, buffm, plen); 
        outlen2 += plen;
        loop += curr_len;
        }
    FILE* outf = stdout;
    if(strlen(outfile))
        {
        outf = fopen(outfile, "wb");
        if(!outf)
            {
            xerr("Cannnot open outfile");
            }
        }
    int retf = fwrite(outptr, outlen2, 1, outf); 
    if(retf != 1)
        {
        xerr("Cannot write to file");
        }
    if(outf != stdout)
        {
        fclose(outf);
        }
        
    /* Release contexts. */
    gcry_sexp_release(rsa_keypair);
    gcry_sexp_release(privk);
    gcry_cipher_close(aes_hd);
    
    zline2(__LINE__, __FILE__);
    zfree(outptr);
    zline2(__LINE__, __FILE__);
    zfree(data_buf);
    zline2(__LINE__, __FILE__);
    zfree(rsa_buf);
  
    zleak();
    return 0;
}

/* EOF */


