/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * eog-collection-control.h.
 *
 * Authors:
 *   Martin Baulig (baulig@suse.de)
 *   Jens Finke (jens@gnome.org)
 *
 * Copyright 2000, SuSE GmbH.
 * Copyright 2001, The Free Software Foundation
 */

#ifndef _EOG_COLLECTION_CONTROL_H_
#define _EOG_COLLECTION_CONTROL_H_

#include <bonobo/bonobo-control.h>

#include "eog-collection-view.h"

G_BEGIN_DECLS
 
#define EOG_TYPE_COLLECTION_CONTROL        (eog_collection_control_get_type ())
#define EOG_COLLECTION_CONTROL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_COLLECTION_CONTROL, EogCollectionControl))
#define EOG_CONTROL_CLASS(k)               (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_COLLECTION_CONTROL, EogCollectionControlClass))

#define EOG_IS_COLLECTION_CONTROL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_COLLECTION_CONTROL))
#define EOG_IS_COLLECTION_CONTROL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_COLLECTION_CONTROL))
#define EOG_COLLECTION_CONTROL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_COLLECTION_CONTROL, EogCollectionControlClass))


typedef struct _EogCollectionControl         EogCollectionControl;
typedef struct _EogCollectionControlClass    EogCollectionControlClass;
typedef struct _EogCollectionControlPrivate  EogCollectionControlPrivate;

struct _EogCollectionControl {
	BonoboControl object;

	EogCollectionControlPrivate *priv;
};

struct _EogCollectionControlClass {
	BonoboControlClass parent_class;
};

GType                 eog_collection_control_get_type  (void);
EogCollectionControl *eog_collection_control_new       (void);
EogCollectionControl *eog_collection_control_construct (EogCollectionControl*);

G_END_DECLS

#endif /* _EOG_COLLECTION_CONTROL_H_ */
