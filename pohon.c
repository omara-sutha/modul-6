#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NAMA 50
#define MAX_ORANG 100

typedef struct NodeAnak {
    int id_anak;
    struct NodeAnak *next;
} NodeAnak;

typedef struct Orang {
    int id;
    char nama[MAX_NAMA];
    int id_ortu;
    NodeAnak *anak_list;
} Orang;

typedef struct Pohon {
    Orang *data[MAX_ORANG];
    int jumlah;
    int next_id;
} Pohon;

Pohon *buat_pohon();
void isi_data_awal(Pohon *p);
int tambah_orang(Pohon *p, const char *nama, int id_ortu);
Orang *cari_id(Pohon *p, int id);
Orang *cari_nama(Pohon *p, const char *nama);
void tambah_anak_ke_ortu(Pohon *p, int id_ortu, int id_anak);
void scan_nama(char *buf);

/* ============================================================
 * VISUAL POHON DI TERMINAL
 * ============================================================
 * Menggambar pohon dengan karakter ASCII/Unicode box-drawing.
 * Prefix:  ""          = root
 *          "├── "      = anak bukan terakhir
 *          "└── "      = anak terakhir
 *          "│   "      = indent lanjutan
 *          "    "      = indent kosong
 * ============================================================ */

/* Hitung jumlah anak langsung */
int jumlah_anak(Orang *o) {
    int n = 0;
    NodeAnak *a = o->anak_list;
    while (a) { n++; a = a->next; }
    return n;
}

/* Cetak pohon secara rekursif dengan box-drawing */
void cetak_pohon_visual(Pohon *p, int id, char *prefix, int is_last) {
    Orang *o = cari_id(p, id);
    if (!o) return;

    /* Cetak baris node ini */
    if (id == p->data[0]->id) {
        /* Root: cetak nama langsung tanpa prefix cabang */
        printf("%s\n", o->nama);
    } else {
        printf("%s%s%s\n",
               prefix,
               is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "   /* └── */
                       : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ",  /* ├── */
               o->nama);
    }

    /* Siapkan prefix untuk anak-anak */
    int n_anak = jumlah_anak(o);
    if (n_anak == 0) return;

    char new_prefix[512];
    if (id == p->data[0]->id) {
        snprintf(new_prefix, sizeof(new_prefix), "%s", prefix);
    } else {
        snprintf(new_prefix, sizeof(new_prefix), "%s%s",
                 prefix,
                 is_last ? "    "
                         : "\xe2\x94\x82   "); /* │   */
    }

    NodeAnak *a = o->anak_list;
    int idx = 0;
    while (a) {
        int last = (idx == n_anak - 1);
        cetak_pohon_visual(p, a->id_anak, new_prefix, last);
        a = a->next;
        idx++;
    }
}

/* Cetak pohon dengan border box di sekeliling */
void tampil_pohon_box(Pohon *p, int id_root) {
    Orang *root = cari_id(p, id_root);
    if (!root) { printf("Node tidak ditemukan.\n"); return; }

    printf("\n");
    /* Header */
    printf(" \xe2\x94\x8c");  /* ┌ */
    for (int i = 0; i < 36; i++) printf("\xe2\x94\x80"); /* ─ */
    printf("\xe2\x94\x90\n"); /* ┐ */

    printf(" \xe2\x94\x82  Pohon Keluarga: %-19s\xe2\x94\x82\n", root->nama); /* │ │ */

    printf(" \xe2\x94\x9c"); /* ├ */
    for (int i = 0; i < 36; i++) printf("\xe2\x94\x80"); /* ─ */
    printf("\xe2\x94\xa4\n"); /* ┤ */

    /* Simpan output pohon ke buffer lewat pipe trick:
       kita cetak langsung tapi dengan padding kiri */
    /* Cetak setiap baris dengan border kiri-kanan */
    /* Karena kita perlu padding, kita gunakan fungsi helper */
    printf(" \xe2\x94\x82  "); cetak_pohon_visual(p, id_root, "  ", 0);

    /* Footer */
    printf(" \xe2\x94\x94"); /* └ */
    for (int i = 0; i < 36; i++) printf("\xe2\x94\x80"); /* ─ */
    printf("\xe2\x94\x98\n"); /* ┘ */
    printf("\n");
}

/* Versi lebih baik: kumpulkan semua baris lalu cetak dengan border */
#define MAX_LINES 200
#define MAX_LINE_LEN 200

static char lines[MAX_LINES][MAX_LINE_LEN];
static int  line_count = 0;

void collect_line(Pohon *p, int id, char *prefix, int is_last) {
    Orang *o = cari_id(p, id);
    if (!o || line_count >= MAX_LINES) return;

    char buf[MAX_LINE_LEN];
    int is_root = (id == p->data[0]->id);

    if (is_root) {
        snprintf(buf, sizeof(buf), "  %s", o->nama);
    } else {
        snprintf(buf, sizeof(buf), "  %s%s%s",
                 prefix,
                 is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "
                         : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ",
                 o->nama);
    }
    strncpy(lines[line_count++], buf, MAX_LINE_LEN - 1);

    int n_anak = jumlah_anak(o);
    if (!n_anak) return;

    char new_prefix[512];
    if (is_root) {
        snprintf(new_prefix, sizeof(new_prefix), "%s", prefix);
    } else {
        snprintf(new_prefix, sizeof(new_prefix), "%s%s",
                 prefix,
                 is_last ? "    " : "\xe2\x94\x82   ");
    }

    NodeAnak *a = o->anak_list;
    int idx = 0;
    while (a) {
        collect_line(p, a->id_anak, new_prefix, idx == n_anak - 1);
        a = a->next;
        idx++;
    }
}

/* Hitung panjang tampilan string (abaikan byte multi-byte UTF-8) */
int display_len(const char *s) {
    int len = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80)       { len++; s++; }
        else if (c < 0xE0)  { len += 2; s += 2; } /* 2-byte: 1 char lebar = 1 kolom */
        else if (c < 0xF0)  { len += 1; s += 3; } /* box-drawing 3-byte = 1 kolom */
        else                { len++; s += 4; }
    }
    return len;
}

/* Panjang visual: hitung kolom sebenarnya */
int vis_len(const char *s) {
    int len = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80) { len++; s++; }
        else if ((c & 0xE0) == 0xC0) { len++; s += 2; }
        else if ((c & 0xF0) == 0xE0) { len++; s += 3; }
        else { len++; s += 4; }
    }
    return len;
}

void tampil_pohon_cantik(Pohon *p, int id_root) {
    line_count = 0;
    collect_line(p, id_root, "", 0);

    /* Cari lebar maksimum */
    int max_w = 30;
    for (int i = 0; i < line_count; i++) {
        int w = vis_len(lines[i]);
        if (w > max_w) max_w = w;
    }
    int box_w = max_w + 4;

    printf("\n");
    /* Top border */
    printf(" \xe2\x94\x8c");
    for (int i = 0; i < box_w; i++) printf("\xe2\x94\x80");
    printf("\xe2\x94\x90\n");

    /* Title */
    Orang *root = cari_id(p, id_root);
    char title[80];
    snprintf(title, sizeof(title), " Pohon Keluarga: %s ", root ? root->nama : "?");
    int tlen = vis_len(title);
    int lpad = (box_w - tlen) / 2;
    int rpad = box_w - tlen - lpad;
    printf(" \xe2\x94\x82");
    for (int i = 0; i < lpad; i++) printf(" ");
    printf("%s", title);
    for (int i = 0; i < rpad; i++) printf(" ");
    printf("\xe2\x94\x82\n");

    /* Separator */
    printf(" \xe2\x94\x9c");
    for (int i = 0; i < box_w; i++) printf("\xe2\x94\x80");
    printf("\xe2\x94\xa4\n");

    /* Isi pohon */
    for (int i = 0; i < line_count; i++) {
        int w = vis_len(lines[i]);
        int pad = box_w - w - 1;
        printf(" \xe2\x94\x82%s", lines[i]);
        for (int j = 0; j < pad; j++) printf(" ");
        printf(" \xe2\x94\x82\n");
    }

    /* Bottom border */
    printf(" \xe2\x94\x94");
    for (int i = 0; i < box_w; i++) printf("\xe2\x94\x80");
    printf("\xe2\x94\x98\n\n");
}

/* Cari & tampilkan jalur */
int cari_jalur(Pohon *p, int id_target, int *jalur, int *panjang) {
    int id_skr = jalur[*panjang - 1];
    Orang *skr = cari_id(p, id_skr);
    if (!skr) return 0;
    if (id_skr == id_target) return 1;
    NodeAnak *a = skr->anak_list;
    while (a) {
        jalur[*panjang] = a->id_anak;
        (*panjang)++;
        if (cari_jalur(p, id_target, jalur, panjang)) return 1;
        (*panjang)--;
        a = a->next;
    }
    return 0;
}

void tampil_jalur(Pohon *p, const char *nama) {
    Orang *target = cari_nama(p, nama);
    if (!target) { printf("'%s' tidak ditemukan.\n", nama); return; }

    int jalur[MAX_ORANG], panjang = 1;
    jalur[0] = p->data[0]->id;
    if (!cari_jalur(p, target->id, jalur, &panjang)) {
        printf("Jalur tidak ditemukan.\n"); return;
    }
    printf("\n Jalur rekursi: ");
    for (int i = 0; i < panjang; i++) {
        Orang *o = cari_id(p, jalur[i]);
        if (o) printf("%s", o->nama);
        if (i < panjang - 1) printf(" \xe2\x86\x92 "); /* → */
    }
    printf("\n\n");
}

/* ============================================================ MENU */
void cetak_menu() {
    printf("====================================\n");
    printf("  Menu Pohon Keluarga (Visual C)   \n");
    printf("====================================\n");
    printf("  1. Tampilkan pohon visual\n");
    printf("  2. Tampilkan pohon dari anggota\n");
    printf("  3. Tambah anggota baru\n");
    printf("  4. Cari jalur ke seseorang\n");
    printf("  5. Lihat semua data (tabel)\n");
    printf("  0. Keluar\n");
    printf("====================================\n");
    printf("Pilihan: ");
}

int main() {
    Pohon *p = buat_pohon();
    isi_data_awal(p);

    printf("\n=== Program Pohon Keluarga Visual ===\n");
    printf("Data awal: %d orang dimuat.\n", p->jumlah);

    char pilihan;
    do {
        cetak_menu();
        scanf(" %c", &pilihan);
        char nama[MAX_NAMA], nama_ortu[MAX_NAMA];

        switch (pilihan) {
        case '1':
            tampil_pohon_cantik(p, p->data[0]->id);
            break;

        case '2':
            printf("Nama root yang ditampilkan: ");
            scan_nama(nama);
            {
                Orang *o = cari_nama(p, nama);
                if (!o) printf("'%s' tidak ditemukan.\n", nama);
                else tampil_pohon_cantik(p, o->id);
            }
            break;

        case '3':
            printf("Nama baru       : "); scan_nama(nama);
            if (cari_nama(p, nama)) { printf("Nama sudah ada!\n"); break; }
            printf("Nama orang tua  : "); scan_nama(nama_ortu);
            {
                int id_ortu = -1;
                if (strlen(nama_ortu) > 0) {
                    Orang *ortu = cari_nama(p, nama_ortu);
                    if (!ortu) { printf("Orang tua '%s' tidak ditemukan!\n", nama_ortu); break; }
                    id_ortu = ortu->id;
                }
                int id_baru = tambah_orang(p, nama, id_ortu);
                printf("Berhasil! '%s' ditambahkan (id=%d).\n", nama, id_baru);
                tampil_pohon_cantik(p, p->data[0]->id);
            }
            break;

        case '4':
            printf("Nama yang dicari: "); scan_nama(nama);
            tampil_jalur(p, nama);
            break;

        case '5':
            printf("\n%-5s %-15s %-10s\n", "ID", "NAMA", "ID_ORTU");
            printf("------------------------------\n");
            for (int i = 0; i < p->jumlah; i++) {
                Orang *o = p->data[i];
                if (o->id_ortu == -1)
                    printf("%-5d %-15s %-10s\n", o->id, o->nama, "(root)");
                else
                    printf("%-5d %-15s %-10d\n", o->id, o->nama, o->id_ortu);
            }
            printf("\n");
            break;

        case '0':
            printf("Keluar.\n");
            break;
        default:
            printf("Pilihan tidak valid.\n");
        }
    } while (pilihan != '0');

    for (int i = 0; i < p->jumlah; i++) {
        NodeAnak *a = p->data[i]->anak_list;
        while (a) { NodeAnak *tmp = a; a = a->next; free(tmp); }
        free(p->data[i]);
    }
    free(p);
    return 0;
}

/* ============================================================ HELPERS */
Pohon *buat_pohon() {
    Pohon *p = (Pohon*)malloc(sizeof(Pohon));
    p->jumlah = 0; p->next_id = 1;
    for (int i = 0; i < MAX_ORANG; i++) p->data[i] = NULL;
    return p;
}

void isi_data_awal(Pohon *p) {
    int id_joni  = tambah_orang(p, "Joni",  -1);
    int id_budi  = tambah_orang(p, "Budi",  id_joni);
    int id_iwan  = tambah_orang(p, "Iwan",  id_joni);
                   tambah_orang(p, "Wati",  id_joni);
                   tambah_orang(p, "Aril",  id_joni);
                   tambah_orang(p, "Omara", id_joni);
    int id_bani  = tambah_orang(p, "Bani",  id_budi);
                   tambah_orang(p, "Joko",  id_budi);
                   tambah_orang(p, "Ani",   id_bani);
                   tambah_orang(p, "Johan", id_bani);
                   tambah_orang(p, "Ari",   id_iwan);
                   tambah_orang(p, "Dara",  id_iwan);
}

int tambah_orang(Pohon *p, const char *nama, int id_ortu) {
    if (p->jumlah >= MAX_ORANG) return -1;
    Orang *baru = (Orang*)malloc(sizeof(Orang));
    baru->id = p->next_id++;
    strncpy(baru->nama, nama, MAX_NAMA - 1);
    baru->nama[MAX_NAMA-1] = '\0';
    baru->id_ortu = id_ortu;
    baru->anak_list = NULL;
    p->data[p->jumlah++] = baru;
    if (id_ortu != -1) tambah_anak_ke_ortu(p, id_ortu, baru->id);
    return baru->id;
}

void tambah_anak_ke_ortu(Pohon *p, int id_ortu, int id_anak) {
    Orang *ortu = cari_id(p, id_ortu);
    if (!ortu) return;
    NodeAnak *n = (NodeAnak*)malloc(sizeof(NodeAnak));
    n->id_anak = id_anak; n->next = NULL;
    if (!ortu->anak_list) { ortu->anak_list = n; return; }
    NodeAnak *cur = ortu->anak_list;
    while (cur->next) cur = cur->next;
    cur->next = n;
}

Orang *cari_id(Pohon *p, int id) {
    for (int i = 0; i < p->jumlah; i++)
        if (p->data[i]->id == id) return p->data[i];
    return NULL;
}

Orang *cari_nama(Pohon *p, const char *nama) {
    for (int i = 0; i < p->jumlah; i++)
        if (strcasecmp(p->data[i]->nama, nama) == 0) return p->data[i];
    return NULL;
}

void scan_nama(char *buf) {
    int idx = 0; char c;
    scanf(" %c", &c);
    while (c != '\n' && idx < MAX_NAMA - 1) { buf[idx++] = c; scanf("%c", &c); }
    buf[idx] = '\0';
}