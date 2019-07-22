/**
 * \file
 */

#include <glib.h>

#include "w32process.h"
#include "w32process-internals.h"
#include "w32file.h"
#include "object.h"
#include "object-internals.h"
#include "class.h"
#include "class-internals.h"
#include "image.h"
#include "utils/mono-proclib.h"
#include "utils/w32api.h"
#include "icall-decl.h"

#define LOGDEBUG(...)
/* define LOGDEBUG(...) g_message(__VA_ARGS__)  */

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) && defined(HOST_WIN32)

static guint32
mono_w32process_get_pid (gpointer handle)
{
	return GetProcessId (handle);
}

static gboolean
mono_w32process_try_get_modules (gpointer process, HMODULE *modules, guint32 size, PDWORD needed)
{
	return EnumProcessModules (process, modules, size, needed);
}

static guint32
mono_w32process_module_get_name (gpointer process, gpointer module, gunichar2 *basename, guint32 size)
{
	return GetModuleBaseNameW (process, (HMODULE)module, basename, size);
}

static guint32
mono_w32process_module_get_filename (gpointer process, gpointer module, gunichar2 *basename, guint32 size)
{
	return GetModuleFileNameExW (process, (HMODULE)module, basename, size);
}

static gboolean
mono_w32process_module_get_information (gpointer process, gpointer module, MODULEINFO *modinfo, guint32 size)
{
	return GetModuleInformation (process, (HMODULE)module, modinfo, size);
}

static gboolean
mono_w32process_get_fileversion_info (const gunichar2 *filename, gpointer *data)
{
	DWORD handle;

	g_assert (data);
	*data = NULL;

	DWORD datasize = GetFileVersionInfoSizeW (filename, &handle);
	if (datasize <= 0)
		return FALSE;

	*data = g_malloc0 (datasize);
	if (!GetFileVersionInfoW (filename, handle, datasize, *data)) {
		g_free (*data);
		return FALSE;
	}

	return TRUE;
}

static gboolean
mono_w32process_ver_query_value (gconstpointer datablock, const gunichar2 *subblock, gpointer *buffer, guint32 *len)
{
	return VerQueryValueW (datablock, subblock, buffer, len);
}

static guint32
mono_w32process_ver_language_name (guint32 lang, gunichar2 *lang_out, guint32 lang_len)
{
	return VerLanguageNameW (lang, lang_out, lang_len);
}

#endif /* G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) && defined(HOST_WIN32) */

static MonoImage *system_image;

static void
stash_system_image (MonoImage *image)
{
	system_image = image;
}

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)

static MonoClass*
get_file_version_info_class (void)
{
	static MonoClass *file_version_info_class;

	if (file_version_info_class)
		return file_version_info_class;

	g_assert (system_image);

	return file_version_info_class = mono_class_load_from_name (
		system_image, "System.Diagnostics", "FileVersionInfo");
}

static MonoClass*
get_process_module_class (void)
{
	static MonoClass *process_module_class;

	if (process_module_class)
		return process_module_class;

	g_assert (system_image);

	return process_module_class = mono_class_load_from_name (
		system_image, "System.Diagnostics", "ProcessModule");
}

static void
process_set_field_ref (MonoObjectHandle obj, const char *fieldname, MonoObjectHandle data)
{
	// FIXME Cache offsets. Per-class.
	MonoClass *klass = mono_handle_class (obj);
	g_assert (klass);

	MonoClassField *field = mono_class_get_field_from_name_full (klass, fieldname, NULL);
	g_assert (field);

	MONO_HANDLE_SET_FIELD_REF (obj, field, data);
}

static void
process_set_field_object (MonoObjectHandle obj, const char *fieldname, MonoObjectHandle data)
{
	LOGDEBUG (g_message ("%s: Setting field %s to object at %p", __func__, fieldname, data));

	process_set_field_ref (obj, fieldname, data);
}

static void
process_set_field_string (MonoObjectHandle obj, const char *fieldname, MonoStringHandle string)
{
	process_set_field_ref (obj, fieldname, MONO_HANDLE_CAST (MonoObject, string));
}

static void
process_set_field_utf16 (MonoObjectHandle obj, MonoStringHandle str, const char *fieldname, const gunichar2 *val, guint32 len, MonoError *error)
{
	LOGDEBUG (g_message ("%s: Setting field %s to [%s]", __func__, fieldname, g_utf16_to_utf8 (val, len, NULL, NULL, NULL)));

	MonoDomain *domain = MONO_HANDLE_DOMAIN (obj);
	g_assert (domain);

	mono_string_new_utf16_assign (str, domain, val, len, error);
	return_if_nok (error);
	process_set_field_string (obj, fieldname, str);
}

static void
process_set_field_utf8 (MonoObjectHandle obj, MonoStringHandle str, const char *fieldname, const char *val, MonoError *error)
{
	LOGDEBUG (g_message ("%s: Setting field %s to [%s]", __func__, fieldname, val));

	MonoDomain *domain = MONO_HANDLE_DOMAIN (obj);
	g_assert (domain);

	mono_string_new_utf8z_assign (str, domain, val, error);
	return_if_nok (error);

	process_set_field_string (obj, fieldname, str);
}

static MonoClassField*
process_resolve_field (MonoObjectHandle obj, const char *fieldname)
{
	// FIXME Cache offsets. Per-class.
	MonoClass *klass = mono_handle_class (obj);
	g_assert (klass);

	MonoClassField *field = mono_class_get_field_from_name_full (klass, fieldname, NULL);
	g_assert (field);

	return field;
}

static void
process_set_field_int (MonoObjectHandle obj, const char *fieldname, guint32 val)
{
	LOGDEBUG (g_message ("%s: Setting field %s to %d", __func__, fieldname, val));

	MONO_HANDLE_SET_FIELD_VAL (obj, guint32, process_resolve_field (obj, fieldname), val);
}

static void
process_set_field_intptr (MonoObjectHandle obj, const char *fieldname, gpointer val)
{
	LOGDEBUG (g_message ("%s: Setting field %s to %p", __func__, fieldname, val));

	MONO_HANDLE_SET_FIELD_VAL (obj, gpointer, process_resolve_field (obj, fieldname), val);
}

static void
process_set_field_bool (MonoObjectHandle obj, const char *fieldname, guint8 val)
{
	LOGDEBUG (g_message ("%s: Setting field %s to %s", __func__, fieldname, val ? "TRUE" : "FALSE"));

	MONO_HANDLE_SET_FIELD_VAL (obj, guint8, process_resolve_field (obj, fieldname), val);
}

#define SFI_COMMENTS		"\\StringFileInfo\\%02X%02X%02X%02X\\Comments"
#define SFI_COMPANYNAME		"\\StringFileInfo\\%02X%02X%02X%02X\\CompanyName"
#define SFI_FILEDESCRIPTION	"\\StringFileInfo\\%02X%02X%02X%02X\\FileDescription"
#define SFI_FILEVERSION		"\\StringFileInfo\\%02X%02X%02X%02X\\FileVersion"
#define SFI_INTERNALNAME	"\\StringFileInfo\\%02X%02X%02X%02X\\InternalName"
#define SFI_LEGALCOPYRIGHT	"\\StringFileInfo\\%02X%02X%02X%02X\\LegalCopyright"
#define SFI_LEGALTRADEMARKS	"\\StringFileInfo\\%02X%02X%02X%02X\\LegalTrademarks"
#define SFI_ORIGINALFILENAME	"\\StringFileInfo\\%02X%02X%02X%02X\\OriginalFilename"
#define SFI_PRIVATEBUILD	"\\StringFileInfo\\%02X%02X%02X%02X\\PrivateBuild"
#define SFI_PRODUCTNAME		"\\StringFileInfo\\%02X%02X%02X%02X\\ProductName"
#define SFI_PRODUCTVERSION	"\\StringFileInfo\\%02X%02X%02X%02X\\ProductVersion"
#define SFI_SPECIALBUILD	"\\StringFileInfo\\%02X%02X%02X%02X\\SpecialBuild"
static const gunichar2 mono_empty_string [ ] = { 0 };
#define EMPTY_STRING		mono_empty_string

typedef struct {
	const char *name;
	const char *id;
} StringTableEntry;

static const StringTableEntry stringtable_entries [] = {
	{ "comments", SFI_COMMENTS },
	{ "companyname", SFI_COMPANYNAME },
	{ "filedescription", SFI_FILEDESCRIPTION },
	{ "fileversion", SFI_FILEVERSION },
	{ "internalname", SFI_INTERNALNAME },
	{ "legalcopyright", SFI_LEGALCOPYRIGHT },
	{ "legaltrademarks", SFI_LEGALTRADEMARKS },
	{ "originalfilename", SFI_ORIGINALFILENAME },
	{ "privatebuild", SFI_PRIVATEBUILD },
	{ "productname", SFI_PRODUCTNAME },
	{ "productversion", SFI_PRODUCTVERSION },
	{ "specialbuild", SFI_SPECIALBUILD }
};

static void
process_module_string_read (MonoObjectHandle filever, MonoStringHandle str, gpointer data, const char *fieldname,
		guchar lang_hi, guchar lang_lo, const gchar *key, MonoError *error)
{
	char *lang_key_utf8 = NULL;
	gunichar2 *lang_key = NULL;
	const gunichar2 *buffer;
	UINT chars;

	lang_key_utf8 = g_strdup_printf (key, lang_lo, lang_hi, 0x04, 0xb0);
	if (!lang_key_utf8)
		goto exit;

	LOGDEBUG (g_message ("%s: asking for [%s]", __func__, lang_key_utf8));

	lang_key = g_utf8_to_utf16 (lang_key_utf8, -1, NULL, NULL, NULL);
	if (!lang_key)
		goto exit;

	if (mono_w32process_ver_query_value (data, lang_key, (gpointer *)&buffer, &chars) && chars > 0) {
		LOGDEBUG (g_message ("%s: found %d chars of [%s]", __func__, chars, g_utf16_to_utf8 (buffer, chars, NULL, NULL, NULL)));
		/* chars includes trailing null */
		chars -= 1;
	} else {
		buffer = EMPTY_STRING;
		chars = 0;
	}
	process_set_field_utf16 (filever, str, fieldname, buffer, chars, error);
exit:
	g_free (lang_key);
	g_free (lang_key_utf8);
}

static void
process_module_stringtable (MonoObjectHandle filever, MonoStringHandle str, gpointer data, guchar lang_hi, guchar lang_lo, MonoError *error)
{
	for (int i = 0; is_ok (error) && i < G_N_ELEMENTS (stringtable_entries); ++i) {
		process_module_string_read (filever, str, data, stringtable_entries [i].name,
			lang_hi, lang_lo, stringtable_entries [i].id, error);
	}
}

static void
mono_w32process_get_fileversion (MonoObjectHandle filever, MonoStringHandle str, const gunichar2 *filename, MonoError *error)
{
	VS_FIXEDFILEINFO *ffi;
	gpointer data = NULL;
	guchar *trans_data;
	gunichar2 *query = NULL;
	UINT ffi_size, trans_size;
	gunichar2 lang_buf [128];
	guint32 lang, lang_count;

	if (!mono_w32process_get_fileversion_info (filename, &data))
		goto cleanup;

	query = g_utf8_to_utf16 ("\\", -1, NULL, NULL, NULL);
	if (query == NULL)
		goto cleanup;

	if (mono_w32process_ver_query_value (data, query, (gpointer *)&ffi, &ffi_size)) {
		#define LOWORD(i32) ((guint16)((i32) & 0xFFFF))
		#define HIWORD(i32) ((guint16)(((guint32)(i32) >> 16) & 0xFFFF))

		LOGDEBUG (g_message ("%s: recording assembly: FileName [%s] FileVersionInfo [%d.%d.%d.%d]",
			__func__, g_utf16_to_utf8 (filename, -1, NULL, NULL, NULL), HIWORD (ffi->dwFileVersionMS),
				LOWORD (ffi->dwFileVersionMS), HIWORD (ffi->dwFileVersionLS), LOWORD (ffi->dwFileVersionLS)));

		process_set_field_int (filever, "filemajorpart", HIWORD (ffi->dwFileVersionMS));
		process_set_field_int (filever, "fileminorpart", LOWORD (ffi->dwFileVersionMS));
		process_set_field_int (filever, "filebuildpart", HIWORD (ffi->dwFileVersionLS));
		process_set_field_int (filever, "fileprivatepart", LOWORD (ffi->dwFileVersionLS));

		process_set_field_int (filever, "productmajorpart", HIWORD (ffi->dwProductVersionMS));
		process_set_field_int (filever, "productminorpart", LOWORD (ffi->dwProductVersionMS));
		process_set_field_int (filever, "productbuildpart", HIWORD (ffi->dwProductVersionLS));
		process_set_field_int (filever, "productprivatepart", LOWORD (ffi->dwProductVersionLS));

		process_set_field_bool (filever, "isdebug", ((ffi->dwFileFlags & ffi->dwFileFlagsMask) & VS_FF_DEBUG) != 0);
		process_set_field_bool (filever, "isprerelease", ((ffi->dwFileFlags & ffi->dwFileFlagsMask) & VS_FF_PRERELEASE) != 0);
		process_set_field_bool (filever, "ispatched", ((ffi->dwFileFlags & ffi->dwFileFlagsMask) & VS_FF_PATCHED) != 0);
		process_set_field_bool (filever, "isprivatebuild", ((ffi->dwFileFlags & ffi->dwFileFlagsMask) & VS_FF_PRIVATEBUILD) != 0);
		process_set_field_bool (filever, "isspecialbuild", ((ffi->dwFileFlags & ffi->dwFileFlagsMask) & VS_FF_SPECIALBUILD) != 0);

		#undef LOWORD
		#undef HIWORD
	}
	g_free (query);

	query = g_utf8_to_utf16 ("\\VarFileInfo\\Translation", -1, NULL, NULL, NULL);
	if (query == NULL)
		goto cleanup;

	if (mono_w32process_ver_query_value (data, query, (gpointer *)&trans_data, &trans_size)) {
		/* use the first language ID we see */
		if (trans_size >= 4) {
 			LOGDEBUG (g_message("%s: %s has 0x%0x 0x%0x 0x%0x 0x%0x", __func__, g_utf16_to_utf8 (filename, -1, NULL, NULL, NULL), trans_data[0], trans_data[1], trans_data[2], trans_data[3]));
			lang = (trans_data[0]) | (trans_data[1] << 8) | (trans_data[2] << 16) | (trans_data[3] << 24);
			/* Only give the lower 16 bits to mono_w32process_ver_language_name, as Windows gets confused otherwise  */
			lang_count = mono_w32process_ver_language_name (lang & 0xFFFF, lang_buf, 128);
			if (lang_count) {
				process_set_field_utf16 (filever, str, "language", lang_buf, lang_count, error);
				goto_if_nok (error, cleanup);
			}
			process_module_stringtable (filever, str, data, trans_data [0], trans_data [1], error);
			goto_if_nok (error, cleanup);
		}
	} else {
		int i;

		for (i = 0; i < G_N_ELEMENTS (stringtable_entries); ++i) {
			/* No strings, so set every field to the empty string */
			process_set_field_utf16 (filever, str, stringtable_entries [i].name, EMPTY_STRING, 0, error);
			goto_if_nok (error, cleanup);
		}

		/* And language seems to be set to en_US according to bug 374600 */
		lang_count = mono_w32process_ver_language_name (0x0409, lang_buf, 128);
		if (lang_count) {
			process_set_field_utf16 (filever, str, "language", lang_buf, lang_count, error);
			goto_if_nok (error, cleanup);
		}
	}

cleanup:
	g_free (query);
	g_free (data);
}

#endif /* #if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) */

void
ves_icall_System_Diagnostics_FileVersionInfo_GetVersionInfo_internal (MonoObjectHandle this_obj,
		const gunichar2 *filename, int filename_length, MonoError *error)
{
	MonoStringHandle str = MONO_HANDLE_CAST (MonoString, mono_new_null ());

	stash_system_image (m_class_get_image (mono_handle_class (this_obj)));

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)

	mono_w32process_get_fileversion (this_obj, str, filename, error);
	return_if_nok (error);

	process_set_field_utf16 (this_obj, str, "filename", filename, filename_length, error);

#else

	g_unsupported_api ("GetFileVersionInfoSize, GetFileVersionInfo, VerQueryValue, VerLanguageName");
	mono_error_set_not_supported (error, G_UNSUPPORTED_API, "GetFileVersionInfoSize, GetFileVersionInfo, VerQueryValue, VerLanguageName");
	SetLastError (ERROR_NOT_SUPPORTED);

#endif
}

#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)

static GPtrArray*
get_domain_assemblies (MonoDomain *domain)
{
	GSList *tmp;
	GPtrArray *assemblies;

	/*
	 * Make a copy of the list of assemblies because we can't hold the assemblies
	 * lock while creating objects etc.
	 */
	assemblies = g_ptr_array_new ();

	mono_domain_assemblies_lock (domain);

	for (tmp = domain->domain_assemblies; tmp; tmp = tmp->next) {
		MonoAssembly *ass = (MonoAssembly *)tmp->data;
		if (m_image_is_fileio_used (ass->image))
			continue;
		g_ptr_array_add (assemblies, ass);
	}

	mono_domain_assemblies_unlock (domain);

	return assemblies;
}

static void
process_add_module (MonoObjectHandle item, MonoObjectHandle filever, MonoStringHandle str,
	HANDLE process, HMODULE mod, const gunichar2 *filename, const gunichar2 *modulename,
	MonoClass *proc_class, MonoError *error)
{
	MODULEINFO modinfo;

	MonoDomain *domain = mono_domain_get ();
	g_assert (domain);

	/* Build a System.Diagnostics.ProcessModule with the data. */
	mono_object_new_assign (item, domain, proc_class, error);
	return_if_nok (error);

	mono_object_new_assign (filever, domain, get_file_version_info_class (), error);
	return_if_nok (error);

	mono_w32process_get_fileversion (filever, str, filename, error);
	return_if_nok (error);

	process_set_field_utf16 (filever, str, "filename", filename, g_utf16_len (filename), error);
	return_if_nok (error);

	if (mono_w32process_module_get_information (process, mod, &modinfo, sizeof (MODULEINFO))) {
		process_set_field_intptr (item, "baseaddr", modinfo.lpBaseOfDll);
		process_set_field_intptr (item, "entryaddr", modinfo.EntryPoint);
		process_set_field_int (item, "memory_size", modinfo.SizeOfImage);
	}

	process_set_field_utf16 (item, str, "filename", filename, g_utf16_len (filename), error);
	return_if_nok (error);

	process_set_field_utf16 (item, str, "modulename", modulename, g_utf16_len (modulename), error);
	return_if_nok (error);

	process_set_field_object (item, "version_info", filever);
}

static void
process_get_assembly_fileversion (MonoObjectHandle filever, MonoAssembly *assembly)
{
	process_set_field_int (filever, "filemajorpart", assembly->aname.major);
	process_set_field_int (filever, "fileminorpart", assembly->aname.minor);
	process_set_field_int (filever, "filebuildpart", assembly->aname.build);
}

static void
process_get_module (MonoObjectHandle item, MonoObjectHandle filever,
	MonoStringHandle str, MonoAssembly *assembly, MonoClass *proc_class, MonoError *error)
{
	MonoDomain *domain = mono_domain_get ();
	const char *modulename = assembly->aname.name;
	char *filename = g_strdup_printf ("[In Memory] %s", modulename);

	/* Build a System.Diagnostics.ProcessModule with the data. */
	mono_object_new_assign (item, domain, proc_class, error);
	goto_if_nok (error, exit);

	mono_object_new_assign (filever, domain, get_file_version_info_class (), error);
	goto_if_nok (error, exit);

	process_get_assembly_fileversion (filever, assembly);
	process_set_field_utf8 (filever, str, "filename", filename, error);
	goto_if_nok (error, exit);
	process_set_field_object (item, "version_info", filever);

	process_set_field_intptr (item, "baseaddr", assembly->image->raw_data);
	process_set_field_int (item, "memory_size", assembly->image->raw_data_len);
	process_set_field_utf8 (item, str, "filename", filename, error);
	goto_if_nok (error, exit);
	process_set_field_utf8 (item, str, "modulename", modulename, error);
exit:
	g_free (filename);
}

#endif /* G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) */

/* Returns an array of System.Diagnostics.ProcessModule */
MonoArrayHandle
ves_icall_System_Diagnostics_Process_GetModules_internal (MonoObjectHandle this_obj, HANDLE process, MonoError *error)
{
#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)
	MonoArrayHandle temp_arr = NULL_HANDLE_ARRAY;
	MonoArrayHandle arr = NULL_HANDLE_ARRAY;
	HMODULE mods [1024];
	gunichar2 filename [MAX_PATH]; // FIXME (MAX_PATH)
	gunichar2 modname [MAX_PATH]; // FIXME (MAX_PATH)
	DWORD needed = 0;
	guint32 count = 0;
	guint32 module_count = 0;
	guint32 assembly_count = 0;
	guint32 i = 0;
	guint32 num_added = 0;
	GPtrArray *assemblies = NULL;

	// Coop handles are created here, once, in order to cut down on
	// handle creation and ease use of loops that would
	// otherwise create handles.
	MonoObjectHandle module = mono_new_null ();
	MonoObjectHandle filever = mono_new_null ();
	MonoStringHandle str = MONO_HANDLE_CAST (MonoString, mono_new_null ());

	stash_system_image (m_class_get_image (mono_handle_class (this_obj)));

	if (mono_w32process_get_pid (process) == mono_process_current_pid ()) {
		assemblies = get_domain_assemblies (mono_domain_get ());
		assembly_count = assemblies->len;
	}

	if (mono_w32process_try_get_modules (process, mods, sizeof (mods), &needed))
		module_count += needed / sizeof (HMODULE);

	count = module_count + assembly_count;
	temp_arr = mono_array_new_handle (mono_domain_get (), get_process_module_class (), count, error);
	return_val_if_nok (error, NULL_HANDLE_ARRAY);

	for (i = 0; i < module_count; i++) {
		if (mono_w32process_module_get_name (process, mods [i], modname, MAX_PATH)
			 && mono_w32process_module_get_filename (process, mods [i], filename, MAX_PATH))
		{
			process_add_module (module, filever, str, process, mods [i], filename, modname, get_process_module_class (), error);
			return_val_if_nok (error, NULL_HANDLE_ARRAY);
			MONO_HANDLE_ARRAY_SETREF (temp_arr, num_added++, module);
		}
	}

	if (assemblies) {
		for (i = 0; i < assembly_count; i++) {
			MonoAssembly *ass = (MonoAssembly *)g_ptr_array_index (assemblies, i);
			process_get_module (module, filever, str, ass, get_process_module_class (), error);
			return_val_if_nok (error, NULL_HANDLE_ARRAY);
			MONO_HANDLE_ARRAY_SETREF (temp_arr, num_added++, module);
		}
		g_ptr_array_free (assemblies, TRUE);
	}

	if (count == num_added)
		return temp_arr;

	/* shorter version of the array */
	arr = mono_array_new_handle (mono_domain_get (), get_process_module_class (), num_added, error);
	return_val_if_nok (error, NULL_HANDLE_ARRAY);

	for (i = 0; i < num_added; i++) {
		MONO_HANDLE_ARRAY_GETREF (module, temp_arr, i);
		MONO_HANDLE_ARRAY_SETREF (arr, i, module);
	}

	return arr;
#else
	g_unsupported_api ("EnumProcessModules");
	mono_error_set_not_supported (error, G_UNSUPPORTED_API, "EnumProcessModules");
	SetLastError (ERROR_NOT_SUPPORTED);
	return NULL_HANDLE_ARRAY;
#endif
}

MonoStringHandle
ves_icall_System_Diagnostics_Process_ProcessName_internal (HANDLE process, MonoError *error)
{
#if G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT)

	gunichar2 name [MAX_PATH]; // FIXME (MAX_PATH)
	HMODULE mod = 0;
	DWORD needed = 0;

	if (!mono_w32process_try_get_modules (process, &mod, sizeof (mod), &needed))
		return NULL_HANDLE_STRING;

	const guint32 len = mono_w32process_module_get_name (process, mod, name, MAX_PATH);

	return len ? mono_string_new_utf16_handle (mono_domain_get (), name, len, error) : NULL_HANDLE_STRING;

#else

	gunichar2 name [MAX_PATH]; // FIXME MAX_PATH
	const guint32 len = GetModuleFileNameW (NULL, name, G_N_ELEMENTS (name));
	if (len == 0)
		return NULL_HANDLE_STRING;
	return mono_string_new_utf16_handle (mono_domain_get (), name, len, error);

#endif /* G_HAVE_API_SUPPORT(HAVE_CLASSIC_WINAPI_SUPPORT) */
}

gint64
ves_icall_System_Diagnostics_Process_GetProcessData (int pid, gint32 data_type, MonoProcessError *error)
{
	g_static_assert (sizeof (MonoProcessError) == sizeof (gint32));
	g_assert (error);
	*error = MONO_PROCESS_ERROR_NONE;
	return mono_process_get_data_with_error (GINT_TO_POINTER (pid), (MonoProcessData)data_type, error);
}

static void
mono_pin_string (MonoStringHandle in_coophandle, MonoStringHandle *out_coophandle, gunichar2 **chars, gsize *length, gchandle_t *gchandle)
{
	*out_coophandle = in_coophandle;
	if (!MONO_HANDLE_IS_NULL (in_coophandle)) {
		*chars = mono_string_handle_pin_chars (in_coophandle, gchandle);
		*length = mono_string_handle_length (in_coophandle);
	}
}

void
mono_createprocess_coop_init (MonoCreateProcessCoop *coop, MonoW32ProcessStartInfoHandle proc_start_info, MonoW32ProcessInfo *process_info)
{
	memset (coop, 0, sizeof (*coop));

#define PIN_STRING(h, x) (mono_pin_string (h, &coop->coophandle.x, &coop->x, &coop->length.x, &coop->gchandle.x))

#define PIN(x) PIN_STRING (MONO_HANDLE_NEW_GET (MonoString, proc_start_info, x), x)
	PIN (filename);
	PIN (arguments);
	PIN (working_directory);
	PIN (verb);
#undef PIN
#define PIN(x) PIN_STRING (MONO_HANDLE_NEW (MonoString, process_info->x), x)
	PIN (username);
	PIN (domain);
#undef PIN
}

static void
mono_unpin_array (gchandle_t *gchandles, gsize count)
{
	for (gsize i = 0; i < count; ++i) {
		mono_gchandle_free_internal (gchandles [i]);
		gchandles [i] = 0;
	}
}

void
mono_createprocess_coop_cleanup (MonoCreateProcessCoop *coop)
{
	mono_unpin_array ((gchandle_t*)&coop->gchandle, sizeof (coop->gchandle) / sizeof (gchandle_t));
	memset (coop, 0, sizeof (*coop));
}
