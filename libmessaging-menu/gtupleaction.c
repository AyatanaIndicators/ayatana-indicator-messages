/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Lars Uebernickel <lars.uebernickel@canonical.com>
 */

#include "gtupleaction.h"

typedef GObjectClass GTupleActionClass;

struct _GTupleAction
{
    GObject        parent;

    gchar         *name;
    GVariantType  *type;
    gboolean       enabled;

    gsize          n_children;
    GVariant     **children;
};

static void action_interface_init (GActionInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GTupleAction, g_tuple_action, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, action_interface_init));

enum
{
  PROP_0,
  PROP_NAME,
  PROP_PARAMETER_TYPE,
  PROP_ENABLED,
  PROP_STATE_TYPE,
  PROP_STATE,
  N_PROPERTIES
};

enum
{
  SIGNAL_ACTIVATE,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPERTIES];
static guint       signal_ids[N_SIGNALS];

static const gchar *
g_tuple_action_get_name (GAction *action)
{
  GTupleAction *tuple = G_TUPLE_ACTION (action);

  return tuple->name;
}

static const GVariantType *
g_tuple_action_get_parameter_type (GAction *action)
{
  return NULL;
}

static const GVariantType *
g_tuple_action_get_state_type (GAction *action)
{
  GTupleAction *tuple = G_TUPLE_ACTION (action);

  return tuple->type;
}

static GVariant *
g_tuple_action_get_state_hint (GAction *action)
{
  return NULL;
}

static gboolean
g_tuple_action_get_enabled (GAction *action)
{
  GTupleAction *tuple = G_TUPLE_ACTION (action);

  return tuple->enabled;
}

static GVariant *
g_tuple_action_get_state (GAction *action)
{
  GTupleAction *tuple = G_TUPLE_ACTION (action);
  GVariant *result;

  result = g_variant_new_tuple (tuple->children, tuple->n_children);
  return g_variant_ref_sink (result);
}

static void
g_tuple_action_set_state (GTupleAction *tuple,
                          GVariant     *state)
{
  int i;

  g_return_if_fail (g_variant_type_is_tuple (g_variant_get_type (state)));

  if (tuple->type == NULL)
    {
      tuple->type = g_variant_type_copy (g_variant_get_type (state));
      tuple->n_children = g_variant_n_children (state);
      tuple->children = g_new0 (GVariant *, tuple->n_children);
    }

  for (i = 0; i < tuple->n_children; i++)
    {
      if (tuple->children[i])
        g_variant_unref (tuple->children[i]);
      tuple->children[i] = g_variant_get_child_value (state, i);
    }

  g_object_notify_by_pspec (G_OBJECT (tuple), properties[PROP_STATE]);
}

static void
g_tuple_action_change_state (GAction  *action,
                             GVariant *value)
{
  GTupleAction *tuple = G_TUPLE_ACTION (action);

  g_return_if_fail (value != NULL);
  g_return_if_fail (g_variant_is_of_type (value, tuple->type));

  g_variant_ref_sink (value);

  /* TODO add a change-state signal similar to GSimpleAction */
  g_tuple_action_set_state (tuple, value);

  g_variant_unref (value);
}

static void
g_tuple_action_activate (GAction  *action,
                         GVariant *parameter)
{
  GTupleAction *tuple = G_TUPLE_ACTION (action);

  g_return_if_fail (parameter == NULL);

  if (tuple->enabled)
    g_signal_emit (tuple, signal_ids[SIGNAL_ACTIVATE], 0, NULL);
}

static void
g_tuple_action_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GAction *action = G_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, g_tuple_action_get_name (action));
      break;

    case PROP_PARAMETER_TYPE:
      g_value_set_boxed (value, g_tuple_action_get_parameter_type (action));
      break;

    case PROP_ENABLED:
      g_value_set_boolean (value, g_tuple_action_get_enabled (action));
      break;

    case PROP_STATE_TYPE:
      g_value_set_boxed (value, g_tuple_action_get_state_type (action));
      break;

    case PROP_STATE:
      g_value_take_variant (value, g_tuple_action_get_state (action));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tuple_action_set_property (GObject      *object,
                             guint          prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GTupleAction *tuple = G_TUPLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_NAME:
      tuple->name = g_value_dup_string (value);
      g_object_notify_by_pspec (object, properties[PROP_NAME]);
      break;

    case PROP_ENABLED:
      tuple->enabled = g_value_get_boolean (value);
      g_object_notify_by_pspec (object, properties[PROP_ENABLED]);
      break;

    case PROP_STATE:
      g_tuple_action_set_state (tuple, g_value_get_variant (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_tuple_action_finalize (GObject *object)
{
  GTupleAction *tuple = G_TUPLE_ACTION (object);
  int i;

  g_free (tuple->name);
  g_variant_type_free (tuple->type);

  for (i = 0; i < tuple->n_children; i++)
    g_variant_unref (tuple->children[i]);

  g_free (tuple->children);

  G_OBJECT_CLASS (g_tuple_action_parent_class)->finalize (object);
}

static void
action_interface_init (GActionInterface *iface)
{
  iface->get_name = g_tuple_action_get_name;
  iface->get_parameter_type = g_tuple_action_get_parameter_type;
  iface->get_state_type = g_tuple_action_get_state_type;
  iface->get_state_hint = g_tuple_action_get_state_hint;
  iface->get_enabled = g_tuple_action_get_enabled;
  iface->get_state = g_tuple_action_get_state;
  iface->change_state = g_tuple_action_change_state;
  iface->activate = g_tuple_action_activate;
}

static void
g_tuple_action_class_init (GTupleActionClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = g_tuple_action_get_property;
  object_class->set_property = g_tuple_action_set_property;
  object_class->finalize = g_tuple_action_finalize;

  properties[PROP_NAME] = g_param_spec_string ("name",
                                               "Name",
                                               "The name of the action",
                                               NULL,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT_ONLY |
                                               G_PARAM_STATIC_STRINGS);

  properties[PROP_PARAMETER_TYPE] = g_param_spec_boxed ("parameter-type",
                                                        "Parameter Type",
                                                        "The variant type passed to activate",
                                                        G_TYPE_VARIANT_TYPE,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS);

  properties[PROP_ENABLED] = g_param_spec_boolean ("enabled",
                                                   "Enabled",
                                                   "Whether the action can be activated",
                                                   TRUE,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_STATIC_STRINGS);

  properties[PROP_STATE_TYPE] = g_param_spec_boxed ("state-type",
                                                    "State Type",
                                                    "The variant type of the state, must be a tuple",
                                                    G_TYPE_VARIANT_TYPE,
                                                    G_PARAM_READABLE |
                                                    G_PARAM_STATIC_STRINGS);

  properties[PROP_STATE] = g_param_spec_variant ("state",
                                                 "State",
                                                 "The state of the action",
                                                 G_VARIANT_TYPE_TUPLE,
                                                 NULL,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);

  signal_ids[SIGNAL_ACTIVATE] = g_signal_new ("activate",
                                              G_TYPE_TUPLE_ACTION,
                                              G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                                              0, NULL, NULL,
                                              g_cclosure_marshal_VOID__VARIANT,
                                              G_TYPE_NONE, 1,
                                              G_TYPE_VARIANT);
}

static void
g_tuple_action_init (GTupleAction *action)
{
  action->enabled = TRUE;
}

GTupleAction *
g_tuple_action_new (const gchar *name,
                    GVariant    *initial_state)
{
  const GVariantType *type;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (initial_state != NULL, NULL);

  type = g_variant_get_type (initial_state);
  g_return_val_if_fail (g_variant_type_is_tuple (type), NULL);

  return g_object_new (G_TYPE_TUPLE_ACTION,
                       "name", name,
                       "state", initial_state,
                       NULL);
}

void
g_tuple_action_set_child (GTupleAction *action,
                          gsize         index,
                          GVariant     *value)
{
  const GVariantType *type;

  g_return_if_fail (G_IS_TUPLE_ACTION (action));
  g_return_if_fail (index < action->n_children);
  g_return_if_fail (value != NULL);

  type = g_variant_get_type (value);
  g_return_if_fail (g_variant_is_of_type (value, type));

  g_variant_unref (action->children[index]);
  action->children[index] = g_variant_ref_sink (value);

  g_object_notify_by_pspec (G_OBJECT (action), properties[PROP_STATE]);
}
