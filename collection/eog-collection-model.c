#include "eog-collection-model.h"
#include "bonobo/bonobo-moniker-util.h"
#include <libgnomevfs/gnome-vfs-directory.h>
#include <libgnome/gnome-macros.h>

#include "eog-collection-marshal.h"

/* Signal IDs */
enum {
	IMAGE_CHANGED,
	IMAGE_ADDED,
	IMAGE_REMOVED,
	SELECTION_CHANGED,
	SELECTED_ALL,
	SELECTED_NONE,
	BASE_URI_CHANGED,
	LAST_SIGNAL
};

static void marshal_interval_notification (GtkObject *object, GtkSignalFunc func, 
					   gpointer data, GtkArg *args);
static guint eog_model_signals[LAST_SIGNAL];

typedef struct {
	EogCollectionModel *model;
	GnomeVFSURI *uri;
	GnomeVFSFileInfo *info;
	GnomeVFSHandle *handle;
} LoadingContext;

struct _EogCollectionModelPrivate {
	/* holds for every id the corresponding cimage */
	GHashTable *id_image_mapping;

	GSList *selected_images;

	/* base uri e.g. from a directory */
	gchar *base_uri;
};

static void eog_collection_model_class_init (EogCollectionModelClass *klass);
static void eog_collection_model_instance_init (EogCollectionModel *object);
static void eog_collection_model_dispose (GObject *object);
static void eog_collection_model_finalize (GObject *object);

GNOME_CLASS_BOILERPLATE (EogCollectionModel, eog_collection_model,
			 GObject, G_TYPE_OBJECT);

static void
free_hash_image (gpointer key, gpointer value, gpointer data)
{
	g_object_unref (G_OBJECT (value));
}

static void
loading_context_free (LoadingContext *ctx)
{
	if (ctx->uri)
		gnome_vfs_uri_unref (ctx->uri);
	ctx->uri = NULL;

	if (ctx->info)
		gnome_vfs_file_info_unref (ctx->info);
	ctx->info = NULL;
	
	if (ctx->handle)
		gnome_vfs_close (ctx->handle);
	ctx->handle = NULL;
	
	g_free (ctx);
}

static void
eog_collection_model_dispose (GObject *obj)
{
	EogCollectionModel *model;
	
	g_return_if_fail (obj != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (obj));

	model = EOG_COLLECTION_MODEL (obj);

	if (model->priv->selected_images)
		g_slist_free (model->priv->selected_images);
	model->priv->selected_images = NULL;

	g_hash_table_foreach (model->priv->id_image_mapping, 
			      (GHFunc) free_hash_image, NULL);
	g_hash_table_destroy (model->priv->id_image_mapping);
	model->priv->id_image_mapping = NULL;

	if (model->priv->base_uri)
		g_free (model->priv->base_uri);
	model->priv->base_uri = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
eog_collection_model_finalize (GObject *obj)
{
	EogCollectionModel *model;
	
	g_return_if_fail (obj != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (obj));

	model = EOG_COLLECTION_MODEL (obj);

	g_free (model->priv);

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
eog_collection_model_instance_init (EogCollectionModel *obj)
{
	EogCollectionModelPrivate *priv;

	priv = g_new0(EogCollectionModelPrivate, 1);
	priv->id_image_mapping = NULL;
	priv->selected_images = NULL;
	priv->base_uri = NULL;
	obj->priv = priv;
}

static void
eog_collection_model_class_init (EogCollectionModelClass *klass)
{
	GObjectClass *object_class = (GObjectClass*) klass;
	
	object_class->dispose = eog_collection_model_dispose;
	object_class->finalize = eog_collection_model_finalize;

	eog_model_signals[IMAGE_CHANGED] =
		g_signal_new ("image-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, image_changed),
			      NULL,
			      NULL,
			      eog_collection_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
	eog_model_signals[IMAGE_ADDED] =
		g_signal_new ("image-added",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, image_added),
			      NULL,
			      NULL,
			      eog_collection_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
	eog_model_signals[IMAGE_REMOVED] =
		g_signal_new ("image-removed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, image_removed),
			      NULL,
			      NULL,
			      eog_collection_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
	eog_model_signals[SELECTION_CHANGED] = 
		g_signal_new ("selection-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, selection_changed),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_INT);
	eog_model_signals[SELECTED_ALL] = 
		g_signal_new ("selected-all",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, selected_all),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	eog_model_signals[SELECTED_NONE] = 
		g_signal_new ("selected-none",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, selected_none),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	eog_model_signals[BASE_URI_CHANGED] = 
		g_signal_new ("base-uri-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogCollectionModelClass, base_uri_changed),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

void
eog_collection_model_construct (EogCollectionModel *model)
{
	g_return_if_fail (model != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	/* init hash table */
	model->priv->id_image_mapping = g_hash_table_new ((GHashFunc) g_direct_hash, 
							  (GCompareFunc) g_direct_equal);
}

EogCollectionModel*
eog_collection_model_new (void)
{
	EogCollectionModel *model;
	
	model = EOG_COLLECTION_MODEL (g_object_new (EOG_TYPE_COLLECTION_MODEL, NULL));

	eog_collection_model_construct (model);

	return model;
}

typedef struct {
	EogCollectionModel *model;
	EogCollectionModelForeachFunc func;
	gpointer data;
	gboolean cont;
} ForeachData;

static void
do_foreach (gpointer key, gpointer value, gpointer user_data)
{
	ForeachData *data = user_data;

	if (data->cont)
		data->cont = data->func (data->model, value,
					 data->data);
}

void
eog_collection_model_foreach (EogCollectionModel *model,
			      EogCollectionModelForeachFunc func,
			      gpointer data)
{
	ForeachData *foreach_data;

	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	foreach_data = g_new0 (ForeachData, 1);
	foreach_data->model = model;
	foreach_data->func = func;
	foreach_data->data = data;
	foreach_data->cont = TRUE;
	g_hash_table_foreach (model->priv->id_image_mapping, do_foreach, 
			      foreach_data);
	g_free (foreach_data);
}

typedef struct {
	EogCollectionModel *model;
	GQuark id;
} RemoveItemData;

static gboolean
remove_item_idle (gpointer user_data)
{
	EogCollectionModelPrivate *priv;
	RemoveItemData *data = user_data;
	CImage *image;

	priv = data->model->priv;

	image = g_hash_table_lookup (priv->id_image_mapping,
				     GINT_TO_POINTER (data->id));
	if (!image) {
		g_warning ("Could not find image %i!", data->id);
		return FALSE;
	}

	g_hash_table_remove (priv->id_image_mapping, GINT_TO_POINTER(data->id));

	if (g_slist_find (priv->selected_images, image)) {
		priv->selected_images = g_slist_remove (priv->selected_images,
							image);
	}

	g_signal_emit_by_name (G_OBJECT (data->model),
			       "image-removed", data->id);

	g_free (data);

	return FALSE;
}

void
eog_collection_model_remove_item (EogCollectionModel *model, GQuark id)
{
	RemoveItemData *data;

	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	/*
	 * We need to do that in the idle loop as there could still be
	 * some loading or a g_hash_table_foreach running.
	 */
	data = g_new0 (RemoveItemData, 1);
	data->model = model;
	data->id = id;
	gtk_idle_add (remove_item_idle, data);
}

static gboolean
directory_visit_cb (const gchar *rel_path,
		    GnomeVFSFileInfo *info,
		    gboolean recursing_will_loop,
		    gpointer data,
		    gboolean *recurse)
{
	CImage *img;
	LoadingContext *ctx;
	GnomeVFSURI *uri;
	EogCollectionModel *model;
	EogCollectionModelPrivate *priv;
	GQuark id;
	static gint count = 0;
	
	ctx = (LoadingContext*) data;
	model = ctx->model;
	priv = model->priv;

	if (g_strncasecmp (info->mime_type, "image/", 6) != 0) {
		return TRUE;
	}
		
	uri = gnome_vfs_uri_append_file_name (ctx->uri, rel_path);	

	img = cimage_new_uri (uri);			
	gnome_vfs_uri_unref (uri);
	id = cimage_get_unique_id (img);

	/* add image infos to internal lists */
	g_hash_table_insert (priv->id_image_mapping,
			     GINT_TO_POINTER (id),
			     img);

	g_signal_emit_by_name (G_OBJECT (model), 
			       "image-added", id);

	if (count++ % 50 == 0)
		while (gtk_events_pending ())
			gtk_main_iteration ();

	return TRUE;
}

static gint
real_dir_loading (LoadingContext *ctx)
{
	EogCollectionModel *model;
	EogCollectionModelPrivate *priv;

	g_return_val_if_fail (ctx->info->type == GNOME_VFS_FILE_TYPE_DIRECTORY, FALSE);

	model = ctx->model;
	priv = model->priv;

	gnome_vfs_directory_visit_uri (ctx->uri,
				       GNOME_VFS_FILE_INFO_DEFAULT |
				       GNOME_VFS_FILE_INFO_FOLLOW_LINKS |
				       GNOME_VFS_FILE_INFO_GET_MIME_TYPE,
				       GNOME_VFS_DIRECTORY_VISIT_DEFAULT,
				       directory_visit_cb,
				       ctx);

	loading_context_free (ctx);
	return FALSE;
}
	

static gint
real_file_loading (LoadingContext *ctx)
{
	EogCollectionModel *model;
	EogCollectionModelPrivate *priv;
	GnomeVFSResult result;

	g_return_val_if_fail (ctx->info->type == GNOME_VFS_FILE_TYPE_REGULAR, FALSE);

	model = ctx->model;
	priv = model->priv;

	result = gnome_vfs_get_file_info_uri (ctx->uri,
					      ctx->info, 
					      GNOME_VFS_FILE_INFO_GET_MIME_TYPE);

	if (result != GNOME_VFS_OK) {
		g_warning ("Error while obtaining file informations.\n");
		return FALSE;
	}
	
	if(g_strncasecmp(ctx->info->mime_type, "image/", 6) == 0) {
		CImage *img;
		GQuark id;

		img = cimage_new_uri (ctx->uri);			
		id = cimage_get_unique_id (img);
		
		/* add image infos to internal lists */
		g_hash_table_insert (priv->id_image_mapping,
				     GINT_TO_POINTER (id),
				     img);
		g_signal_emit_by_name (G_OBJECT (model), 
				       "image-added", id);
	}

	loading_context_free (ctx);

	return FALSE;
}


static LoadingContext*
prepare_context (EogCollectionModel *model, const gchar *text_uri) 
{
	LoadingContext *ctx;
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;

	ctx = g_new0 (LoadingContext, 1);
	ctx->uri = gnome_vfs_uri_new (text_uri);

#ifdef COLLECTION_DEBUG
	g_message ("Prepare context for URI: %s", text_uri);
#endif

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info_uri (ctx->uri, info,
					      GNOME_VFS_FILE_INFO_DEFAULT |
					      GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (result != GNOME_VFS_OK) {
		loading_context_free (ctx);
		return NULL;
	}

	ctx->info = info;
	ctx->handle = 0;
	ctx->model = model;

	return ctx;
}

void
eog_collection_model_set_uri (EogCollectionModel *model, 
			      const gchar *text_uri)
{
	EogCollectionModelPrivate *priv;
	LoadingContext *ctx;

	g_return_if_fail (model != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));
	g_return_if_fail (text_uri != NULL);
	
	priv = model->priv;

	ctx = prepare_context (model, text_uri);
	
	if (ctx != NULL) {
		if (ctx->info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
			gtk_idle_add ((GtkFunction) real_dir_loading, ctx);
		else if (ctx->info->type == GNOME_VFS_FILE_TYPE_REGULAR)
			gtk_idle_add ((GtkFunction) real_file_loading, ctx);
		else {
			loading_context_free (ctx);
			g_warning (_("Can't handle URI: %s"), text_uri);
			return;
		}
	} else {
		g_warning (_("Can't handle URI: %s"), text_uri);
		return;
	}	
	
	if (priv->base_uri == NULL) {
		priv->base_uri = g_strdup (gnome_vfs_uri_get_path (ctx->uri));
	} 
	else {
		g_free (priv->base_uri);
		priv->base_uri = g_strdup("multiple");
	}
	g_signal_emit_by_name (G_OBJECT (model), "base-uri-changed");
}

void 
eog_collection_model_set_uri_list (EogCollectionModel *model,
				   GList *uri_list)
{
	GList *node;
	LoadingContext *ctx;
	gchar *text_uri;

	g_return_if_fail (model != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	node = uri_list;

	while (node != NULL) {
		text_uri = (gchar*) node->data;
		ctx = prepare_context (model, text_uri);

		if (ctx != NULL) {
			if (ctx->info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
				gtk_idle_add ((GtkFunction) real_dir_loading, ctx);
			else if (ctx->info->type == GNOME_VFS_FILE_TYPE_REGULAR)
				gtk_idle_add ((GtkFunction) real_file_loading, ctx);
			else {
				loading_context_free (ctx);
				g_warning (_("Can't handle URI: %s"), text_uri);
				return;
			}
		} else {
			g_warning (_("Can't handle URI: %s"), text_uri);
			return;
		}	

		node = node->next;
	}
	
	if (model->priv->base_uri != NULL)
		g_free (model->priv->base_uri);
	model->priv->base_uri = g_strdup ("multiple");
	g_signal_emit_by_name (G_OBJECT (model), "base-uri-changed");
}

gint
eog_collection_model_get_length (EogCollectionModel *model)
{
	g_return_val_if_fail (model != NULL, 0);
	g_return_val_if_fail (EOG_IS_COLLECTION_MODEL (model), 0);

	return g_hash_table_size (model->priv->id_image_mapping);
}


CImage*
eog_collection_model_get_image (EogCollectionModel *model,
				GQuark id)
{
	g_return_val_if_fail (model != NULL, NULL);
	g_return_val_if_fail (EOG_IS_COLLECTION_MODEL (model), NULL);

	return CIMAGE (g_hash_table_lookup (model->priv->id_image_mapping,
					    GINT_TO_POINTER (id)));
}

gchar*
eog_collection_model_get_uri (EogCollectionModel *model,
			      GQuark id)
{
	CImage *img;
	GnomeVFSURI *uri;
	char *txt_uri;

	g_return_val_if_fail (model != NULL, NULL);
	g_return_val_if_fail (EOG_IS_COLLECTION_MODEL (model), NULL);

	img = eog_collection_model_get_image (model, id);
	if (img == NULL) return NULL;
	
	uri = cimage_get_uri (img);
	txt_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (uri);
	
	return txt_uri;
}

static gboolean
add_image_to_selection_list (EogCollectionModel *model, CImage *img, gpointer data)
{
	EogCollectionModelPrivate *priv;

	priv = model->priv;

	priv->selected_images = g_slist_append (priv->selected_images, img);

	return TRUE;
} 

static void
select_all_images (EogCollectionModel *model)
{
	EogCollectionModelPrivate *priv;

	priv = model->priv;

	if (priv->selected_images != NULL) {
		g_slist_free (priv->selected_images);
		priv->selected_images = NULL;
	}

	eog_collection_model_foreach (model, add_image_to_selection_list, NULL);

	g_signal_emit_by_name (G_OBJECT (model), "selected-all");
}

static void
unselect_all_images (EogCollectionModel *model)
{
	EogCollectionModelPrivate *priv;
	GSList *node;
	CImage *img;
	gboolean single_change_signal;

	priv = model->priv;

	single_change_signal = (eog_collection_model_get_selected_length (model) < 
				(eog_collection_model_get_length (model) / 2));

	for (node = priv->selected_images; node; node = node->next) {
		g_return_if_fail (IS_CIMAGE (node->data));

		img = CIMAGE (node->data);

		cimage_set_select_status (img, FALSE);

		if (single_change_signal) {
			g_signal_emit_by_name (G_OBJECT (model), "selection-changed",
					       cimage_get_unique_id (img));
		}
	}

	if (priv->selected_images)
		g_slist_free (priv->selected_images);
	priv->selected_images = NULL;

	if (!single_change_signal) {
		g_signal_emit_by_name (G_OBJECT (model), "selected-none");
	}
}

void
eog_collection_model_set_select_status (EogCollectionModel *model,
					GQuark id, gboolean status)
{
	CImage *image;

	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	image = eog_collection_model_get_image (model, id);
	if (status == cimage_is_selected (image))
		return;

	cimage_set_select_status (image, status);
	if (status)
		model->priv->selected_images = g_slist_append (
					model->priv->selected_images, image);
	else
		model->priv->selected_images = g_slist_remove (
					model->priv->selected_images, image);

	g_signal_emit_by_name (G_OBJECT (model), "selection-changed",
			       cimage_get_unique_id (image));
}

void
eog_collection_model_set_select_status_all (EogCollectionModel *model, 
					    gboolean status)
{
	g_return_if_fail (model != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	if (status) 
		select_all_images (model);
	else
		unselect_all_images (model);
}


void eog_collection_model_toggle_select_status (EogCollectionModel *model,
						GQuark id)
{
	EogCollectionModelPrivate *priv;
	CImage *image;

	g_return_if_fail (model != NULL);
	g_return_if_fail (EOG_IS_COLLECTION_MODEL (model));

	priv = model->priv;

	image = eog_collection_model_get_image (model, id);
	cimage_toggle_select_status (image);
	if (cimage_is_selected (image)) {
		priv->selected_images = 
			g_slist_append (priv->selected_images,
					image);
	} else {
		priv->selected_images = 
			g_slist_remove (priv->selected_images,
					image);
	}

	g_signal_emit_by_name (G_OBJECT (model), "selection-changed",
			       cimage_get_unique_id (image));
}


gchar*
eog_collection_model_get_base_uri (EogCollectionModel *model)
{
	g_return_val_if_fail (model != NULL, NULL);
	g_return_val_if_fail (EOG_IS_COLLECTION_MODEL (model), NULL);

	if (!model->priv->base_uri) 
		return NULL;
	else
		return model->priv->base_uri;
}


gint
eog_collection_model_get_selected_length (EogCollectionModel *model)
{
	g_return_val_if_fail (model != NULL, 0);
	g_return_val_if_fail (EOG_IS_COLLECTION_MODEL (model), 0);
	
	return g_slist_length (model->priv->selected_images);
}

CImage*
eog_collection_model_get_selected_image (EogCollectionModel *model)
{
	g_return_val_if_fail (model != NULL, 0);
	g_return_val_if_fail (EOG_IS_COLLECTION_MODEL (model), 0);
	
	if (eog_collection_model_get_selected_length (model) == 1) {
		return CIMAGE (model->priv->selected_images->data);
	} else
		return NULL;
}
