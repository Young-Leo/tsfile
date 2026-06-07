/*
 * tsfile_labview_lv.h - LabVIEW Import-Wizard friendly header.
 *
 * Same C ABI as tsfile_labview.h, but with EVERY type fully expanded to a
 * primitive C type. We deliberately use NO typedef and NO stdint.h names
 * (uint8_t / int32_t / int64_t / LV_Handle / ...), because the LabVIEW
 * "Import Shared Library" wizard mis-resolves typedef'd 64-bit stdint names
 * and turns "int64_t" parameters into "void*" (which corrupts the 32-bit
 * cdecl stack). Primitive types map cleanly and match the 32-bit mingw ABI:
 *
 *   unsigned char       -> U8
 *   int                 -> I32   (== the old LV_Status / int32_t)
 *   unsigned int        -> U32   (== the old uint32_t)
 *   long long           -> I64   (== the old int64_t)
 *   unsigned long long  -> U64   (== the old uint64_t / LV_Handle, opaque)
 *   float               -> SGL
 *   double              -> DBL
 *
 * Do NOT use this header to compile the DLL; it is only for the LabVIEW
 * import wizard. The DLL was built from tsfile_labview.h.
 */

#ifndef TSFILE_LABVIEW_LV_H_
#define TSFILE_LABVIEW_LV_H_

/* Data type codes. */
#define LV_TYPE_BOOLEAN 0
#define LV_TYPE_INT32 1
#define LV_TYPE_INT64 2
#define LV_TYPE_FLOAT 3
#define LV_TYPE_DOUBLE 4
#define LV_TYPE_STRING 11

/* Column categories. */
#define LV_CAT_TAG 0
#define LV_CAT_FIELD 1
#define LV_CAT_ATTRIBUTE 2

/* ===================== global configuration ===================== */
int lv_tsfile_set_global_compression(unsigned char compression);
int lv_tsfile_set_datatype_encoding(unsigned char dtype, unsigned char enc);

/* ===================== schema builder ===================== */
unsigned long long lv_tsfile_schema_builder_new(const char* table_name);
int lv_tsfile_schema_builder_add_column(unsigned long long builder,
                                        const char* name,
                                        unsigned char data_type,
                                        unsigned char category);
void lv_tsfile_schema_builder_free(unsigned long long builder);

/* ===================== writer ===================== */
int lv_tsfile_writer_open(const char* path, unsigned long long schema_builder,
                          unsigned long long mem_threshold_bytes,
                          unsigned long long* out_writer);
int lv_tsfile_writer_write(unsigned long long writer, unsigned long long tablet);
int lv_tsfile_writer_close(unsigned long long writer);

/* ===================== tablet builder ===================== */
unsigned long long lv_tsfile_tablet_new(unsigned int max_rows);
int lv_tsfile_tablet_add_column(unsigned long long tablet, const char* name,
                                unsigned char dtype);
int lv_tsfile_tablet_finalize_columns(unsigned long long tablet);

int lv_tsfile_tablet_set_timestamp(unsigned long long tablet, unsigned int row,
                                   long long ts);
int lv_tsfile_tablet_set_i32(unsigned long long tablet, unsigned int row,
                             unsigned int col, int v);
int lv_tsfile_tablet_set_i64(unsigned long long tablet, unsigned int row,
                             unsigned int col, long long v);
int lv_tsfile_tablet_set_f32(unsigned long long tablet, unsigned int row,
                             unsigned int col, float v);
int lv_tsfile_tablet_set_f64(unsigned long long tablet, unsigned int row,
                             unsigned int col, double v);
int lv_tsfile_tablet_set_bool(unsigned long long tablet, unsigned int row,
                              unsigned int col, int v);
int lv_tsfile_tablet_set_str(unsigned long long tablet, unsigned int row,
                             unsigned int col, const char* v, int len);
void lv_tsfile_tablet_free(unsigned long long tablet);

/* ===================== reader ===================== */
int lv_tsfile_reader_open(const char* path, unsigned long long* out_reader);
int lv_tsfile_reader_close(unsigned long long reader);

/* columns: newline ('\n') separated list, e.g. "id1\ns1\ns2". */
int lv_tsfile_query_table(unsigned long long reader, const char* table_name,
                          const char* columns_newline_separated,
                          long long start_time, long long end_time,
                          unsigned long long* out_result_set);

/* ===================== result set ===================== */
int lv_tsfile_rs_next(unsigned long long rs, int* out_err);
int lv_tsfile_rs_column_count(unsigned long long rs);
unsigned char lv_tsfile_rs_column_type(unsigned long long rs, unsigned int col);
int lv_tsfile_rs_is_null(unsigned long long rs, unsigned int col);
int lv_tsfile_rs_get_i32(unsigned long long rs, unsigned int col);
long long lv_tsfile_rs_get_i64(unsigned long long rs, unsigned int col);
float lv_tsfile_rs_get_f32(unsigned long long rs, unsigned int col);
double lv_tsfile_rs_get_f64(unsigned long long rs, unsigned int col);
int lv_tsfile_rs_get_bool(unsigned long long rs, unsigned int col);
int lv_tsfile_rs_get_str(unsigned long long rs, unsigned int col, char* out_buf,
                         int buf_size, int* out_actual_len);
void lv_tsfile_rs_free(unsigned long long rs);

#endif /* TSFILE_LABVIEW_LV_H_ */
