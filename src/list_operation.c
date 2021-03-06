/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2018 Maxime DOYEN
 *
 *  This file is part of HomeBank.
 *
 *  HomeBank is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  HomeBank is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "homebank.h"

#include "list_operation.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


//debug
//extern gboolean minor_active;


/* This is not pretty. Of course you can also use a
   *  separate compare function for each sort ID value */

static gint list_txn_sort_iter_compare_strings(gchar *s1, gchar *s2)
{
	return hb_string_utf8_compare(s1, s2);
}


static   gint
  list_txn_sort_iter_compare_func (GtkTreeModel *model,
                          GtkTreeIter  *a,
                          GtkTreeIter  *b,
                          gpointer      userdata)
  {
    gint sortcol = GPOINTER_TO_INT(userdata);
    gint retval = 0;
	Transaction *ope1, *ope2;
	gdouble tmpval = 0;

	gtk_tree_model_get(model, a, LST_DSPOPE_DATAS, &ope1, -1);
	gtk_tree_model_get(model, b, LST_DSPOPE_DATAS, &ope2, -1);

    switch (sortcol)
    {
		case LST_DSPOPE_STATUS:
			if(!(retval = (ope1->flags & OF_ADDED) - (ope2->flags & OF_ADDED) ) )
			{
				retval = (ope1->flags & OF_CHANGED) - (ope2->flags & OF_CHANGED);
			}
			break;

		case LST_DSPOPE_DATE:
 			if(! (retval = ope1->date - ope2->date) )
			{
				//g_print("sort on balance d1=%d, d2=%d %f %f\n", ope1->date, ope2->date, ope1->balance , ope2->balance);

				tmpval = ope1->pos - ope2->pos;
				retval = tmpval > 0 ? 1 : -1;
			}
			//g_print("ret=%d\n", ret);
			break;

		case LST_DSPOPE_ACCOUNT:
			{
			Account *a1, *a2;

				a1 = da_acc_get(ope1->kacc);
				a2 = da_acc_get(ope2->kacc);
				retval = list_txn_sort_iter_compare_strings((a1 != NULL) ? a1->name : NULL, (a2 != NULL) ? a2->name : NULL);
			}
			break;

		case LST_DSPOPE_INFO:
			if(!(retval = ope1->paymode - ope2->paymode))
			{
				retval = list_txn_sort_iter_compare_strings(ope1->info, ope2->info);
			}
			break;

		case LST_DSPOPE_PAYEE:
			{
			//#1721980
			gchar *name1 = NULL;
			gchar *name2 = NULL;

				if( ope1->paymode == PAYMODE_INTXFER )
				{
				Account *acc = da_acc_get(ope1->kxferacc);
					name1 = (acc != NULL) ? acc->name : NULL;
				}
				else
				{
				Payee *pay = da_pay_get(ope1->kpay);
					name1 = (pay != NULL) ? pay->name : NULL;
				}

				if( ope2->paymode == PAYMODE_INTXFER )
				{
				Account *acc = da_acc_get(ope2->kxferacc);
					name2 = (acc != NULL) ? acc->name : NULL;
				}
				else
				{
				Payee *pay = da_pay_get(ope2->kpay);
					name2 = (pay != NULL) ? pay->name : NULL;
				}

				retval = list_txn_sort_iter_compare_strings(name1, name2);
			}
			break;

		case LST_DSPOPE_MEMO:
				retval = list_txn_sort_iter_compare_strings(ope1->memo, ope2->memo);
			break;

		case LST_DSPOPE_CLR:
			retval = ope1->status - ope2->status;
			break;

		case LST_DSPOPE_AMOUNT:
		case LST_DSPOPE_EXPENSE:
		case LST_DSPOPE_INCOME:
			tmpval = ope1->amount - ope2->amount;
			retval = tmpval > 0 ? 1 : -1;
			break;

		case LST_DSPOPE_CATEGORY:
			{
			Category *c1, *c2;

				c1 = da_cat_get(ope1->kcat);
				c2 = da_cat_get(ope2->kcat);
				if( c1 != NULL && c2 != NULL )
				{
					retval = list_txn_sort_iter_compare_strings(c1->fullname, c2->fullname);
				}
			}
			break;

		case LST_DSPOPE_TAGS:
		{
		gchar *t1, *t2;

			t1 = tags_tostring(ope1->tags);
			t2 = tags_tostring(ope2->tags);
			retval = list_txn_sort_iter_compare_strings(t1, t2);
			g_free(t2);
			g_free(t1);
		}
		break;

		default:
			g_return_val_if_reached(0);
    }

    return retval;
}


static void list_txn_eval_future(GtkCellRenderer *renderer, Transaction *txn)
{
	if(txn->date > GLOBALS->today)
	{
		g_object_set(renderer, 
		//	"scale-set", TRUE, 
			"scale", 0.8,
		//	"style-set", TRUE,
			"style",	PANGO_STYLE_OBLIQUE,
		NULL);
	}
	else
	{
		g_object_set(renderer, "scale-set", FALSE, "style-set", FALSE,
		NULL);
	}
	
	if( txn->marker == TXN_MARK_DUPDST )
	{
		g_object_set(renderer, 
		//	"strikethrough-set", TRUE,
			"strikethrough", TRUE,
			NULL);
	}
	else
	{
		g_object_set(renderer, "strikethrough-set", FALSE,
		NULL);
	}

	if( txn->marker == TXN_MARK_DUPSRC )
	{
		g_object_set(renderer, 
		//	"weight-set", TRUE,
			"weight", PANGO_WEIGHT_BOLD,
			NULL);
	}
	else
	{
		g_object_set(renderer, "weight-set", FALSE,
		NULL);
	}


	
}


/*
** date cell function
*/
static void list_txn_status_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *txn;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &txn, -1);

	/*
		stat[0] = ( entry->ope_Flags & OF_ADDED  ) ? data->istatus[2] : data->istatus[0];
		stat[1] = ( entry->ope_Flags & OF_CHANGED) ? data->istatus[3] : data->istatus[0];
		stat[2] = ( entry->ope_Flags & OF_VALID  ) ? data->istatus[4] : data->istatus[0];
		if( entry->ope_Flags & OF_REMIND ) stat[2] = data->istatus[1];
	*/

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			iconname = ( txn->flags & OF_AUTO  ) ? ICONNAME_HB_OPE_AUTO : ( txn->flags & OF_ADDED ) ? ICONNAME_HB_OPE_NEW : NULL;
			break;
		case 2:
			iconname = ( txn->flags & OF_CHANGED  ) ? ICONNAME_HB_OPE_EDIT : NULL;
			break;
		case 3:
			iconname = ( txn->marker == TXN_MARK_DUPDST ) ? ICONNAME_HB_OPE_SIMILAR : NULL;
			break;
		/*case 3:
			if( entry->flags & OF_VALID )
				iconname = ICONNAME_HB_OPE_VALID;
			else
			{
				if( entry->flags & OF_REMIND )
					iconname = ICONNAME_HB_OPE_REMIND;
			}
			break;*/
	}
	g_object_set(renderer, "icon-name", iconname, NULL);
}


/*
** account cell function
*/
static void list_txn_account_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;
Account *acc;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);

	acc = da_acc_get(ope->kacc);
	if( acc )
	{
		g_object_set(renderer, "text", acc->name, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);
}

/*
** date cell function
*/
static void list_txn_date_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;
gchar buffer[256];
GDate date;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	list_txn_eval_future(renderer, ope);

	if(ope->date > 0)
	{
		g_date_set_julian (&date, ope->date);
		g_date_strftime (buffer, 256-1, PREFS->date_format, &date);
		#if MYDEBUG
		gchar *ds = g_strdup_printf ("%s [%02d]", buffer, ope->pos );
    	g_object_set(renderer, "text", ds, NULL);
		g_free(ds);
		#else
    	g_object_set(renderer, "text", buffer, NULL);
		#endif
	}
    else
    	g_object_set(renderer, "text", "????", NULL);
}

/*
** info cell function
*/
static void list_txn_info_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			g_object_set(renderer, "icon-name", get_paymode_icon_name(ope->paymode), NULL);
			break;
		case 2:
			list_txn_eval_future(renderer, ope);
			g_object_set(renderer, "text", ope->info, NULL);
			break;
	}
}

/*
** payee cell function
*/
static void list_txn_payee_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	list_txn_eval_future(renderer, ope);

	//#926782
	if(ope->paymode == PAYMODE_INTXFER)
	{
	Account *acc = da_acc_get(ope->kxferacc);
		
		g_object_set(renderer, "text", (acc != NULL) ? acc->name : "", NULL);
	}
	else
	{
	Payee *pay = da_pay_get(ope->kpay);
		
		g_object_set(renderer, "text", pay != NULL ? pay->name : "", NULL);
	}
}


/*
** tags cell function
*/
static void list_txn_tags_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;
gchar *str;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	list_txn_eval_future(renderer, ope);

	if(ope->tags != NULL)
	{
		str = tags_tostring(ope->tags);
		g_object_set(renderer, "text", str, NULL);
		g_free(str);
	}
	else
		g_object_set(renderer, "text", "", NULL);


}


/*
** memo cell function
*/
static void list_txn_memo_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	list_txn_eval_future(renderer, ope);
	
    g_object_set(renderer, "text", ope->memo, NULL);
}


/*
** clr cell function
*/
static void list_txn_clr_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;
gchar *iconname = NULL;
//const gchar *c = "";
	
	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	switch(ope->status)
	{
		/*case TXN_STATUS_CLEARED: c = "c"; break;
		case TXN_STATUS_RECONCILED: c = "R"; break;
		case TXN_STATUS_REMIND: c = "!"; break;*/
		case TXN_STATUS_CLEARED:	iconname = ICONNAME_HB_OPE_CLEARED; break;
		case TXN_STATUS_RECONCILED: iconname = ICONNAME_HB_OPE_RECONCILED; break;
		case TXN_STATUS_REMIND:     iconname = ICONNAME_HB_OPE_REMIND; break;
		
	}

	//g_object_set(renderer, "text", c, NULL);
	g_object_set(renderer, "icon-name", iconname, NULL);
	
}


/*
** amount cell function
*/
static void list_txn_amount_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;
gint column = GPOINTER_TO_INT(user_data);
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint type;
gdouble amount;
gchar *color;

	// get the transaction
	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	list_txn_eval_future(renderer, ope);

	if(column == LST_DSPOPE_BALANCE)
		amount = ope->balance;
	else
		amount = ope->amount;

	if(column == LST_DSPOPE_INCOME || column == LST_DSPOPE_EXPENSE)
	{
		type = (ope->flags & OF_INCOME) ? LST_DSPOPE_INCOME : LST_DSPOPE_EXPENSE;
		if( type != column)
		{
			g_object_set(renderer, "markup", NULL, NULL);
			return;
		}
	}

	//if(amount != 0)
	//{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, ope->kcur, GLOBALS->minor);

		color = get_normal_color_amount(amount);
		if( (column == LST_DSPOPE_BALANCE) && (ope->overdraft == TRUE) && (PREFS->custom_colors == TRUE) )
		{
			color = PREFS->color_warn;
		}

		g_object_set(renderer,
			"foreground",  color,
			"text", buf,
			NULL);
	//}

	
}


/*
** category cell function
*/
static void list_txn_category_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Transaction *ope;
Category *cat;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &ope, -1);
	list_txn_eval_future(renderer, ope);

	if(ope->flags & OF_SPLIT)
	{
		g_object_set(renderer, "text", _("- split -"), NULL);
	}
	else
	{
		cat = da_cat_get(ope->kcat);
		if( cat != NULL )
		{
			g_object_set(renderer, "text", cat->fullname, NULL);
		}
		else
			g_object_set(renderer, "text", "", NULL);

	}
	
}


/* = = = = = = = = = = = = = = = = */

GString *list_txn_to_string(GtkTreeView *treeview, gboolean clipboard)
{
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
GString *node;
const gchar *format;
Transaction *ope;
gchar datebuf[16];
gchar *info, *payeename, *categoryname;
Payee *payee;
Category *category;
gchar *tags;
char amountbuf[G_ASCII_DTOSTR_BUF_SIZE];

	node = g_string_new(NULL);

	//title line
	if(clipboard)
		g_string_append (node, "date\tpaymode\tinfo\tpayee\tmemo\tamount\tcategory\ttags\n");
	else
		g_string_append (node, "date;paymode;info;payee;memo;amount;category;tags\n");
	
	model = gtk_tree_view_get_model(treeview);

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		gtk_tree_model_get (model, &iter,
			LST_DSPOPE_DATAS, &ope,
			-1);

		hb_sprint_date(datebuf, ope->date);

		info = ope->info;
		if(info == NULL) info = "";
		payee = da_pay_get(ope->kpay);
		payeename = (payee->name == NULL) ? "" : payee->name;
		category = da_cat_get(ope->kcat);
		categoryname = (category->name == NULL) ? NULL : category->fullname;
		tags = tags_tostring(ope->tags);

		//#793719
		//g_ascii_dtostr (amountbuf, sizeof (amountbuf), ope->amount);
		//#1750257 use locale numdigit
		//g_ascii_formatd (amountbuf, sizeof (amountbuf), "%.2f", ope->amount);
		g_snprintf(amountbuf, sizeof (amountbuf), "%.2f", ope->amount);

		DB( g_print("amount = %f '%s'\n", ope->amount, amountbuf) );

		format = (clipboard == TRUE) ? "%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\n" : "%s;%d;%s;%s;%s;%s;%s;%s\n";
		g_string_append_printf(node, format,
				datebuf,
				ope->paymode,
				info,
				payeename,
				ope->memo,
				amountbuf,
				categoryname != NULL ? categoryname : "",
				tags != NULL ? tags : ""
				);

		//leak
		g_free(tags);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );
	
	return node;
}


gboolean list_txn_column_id_isvisible(GtkTreeView *treeview, gint sort_id)
{
GtkTreeViewColumn *column;
gint n, id;

	for(n=0; n < NUM_LST_DSPOPE-1 ; n++ )   // -1 cause account not to be processed
	{
		column = gtk_tree_view_get_column (treeview, n);
		if(column == NULL)
			continue;

		if( gtk_tree_view_column_get_visible(column) )
		{
			id = gtk_tree_view_column_get_sort_column_id (column);
			if( sort_id == id )
				return TRUE;
		}
	}

	return FALSE;
}


static GtkTreeViewColumn *list_txn_get_column(GList *list, gint search_id)
{
GtkTreeViewColumn *column = NULL;
GList *tmp;
gint id;

	tmp = g_list_first(list);
	while (tmp != NULL)
	{
		id = gtk_tree_view_column_get_sort_column_id(tmp->data);
		if( search_id == id )
		{
			column = tmp->data;
			break;
		}
		tmp = g_list_next(tmp);
	}
	return column;
}


guint list_txn_get_quicksearch_column_mask(GtkTreeView *treeview)
{
GtkTreeViewColumn *column;
guint n, mask;
gint id;

	mask = 0;
	for(n=0; n < NUM_LST_DSPOPE-1 ; n++ )   // -1 cause account not to be processed
	{
		column = gtk_tree_view_get_column (treeview, n);
		if(column == NULL)
			continue;

		if( gtk_tree_view_column_get_visible(column) )
		{
			id = gtk_tree_view_column_get_sort_column_id (column);
			switch(id)
			{
				case LST_DSPOPE_MEMO: mask |= FLT_QSEARCH_MEMO; break;
				case LST_DSPOPE_INFO: mask |= FLT_QSEARCH_INFO; break;
				case LST_DSPOPE_PAYEE: mask |= FLT_QSEARCH_PAYEE; break;
				case LST_DSPOPE_CATEGORY: mask |= FLT_QSEARCH_CATEGORY; break;
				case LST_DSPOPE_TAGS: mask |= FLT_QSEARCH_TAGS; break;
				case LST_DSPOPE_AMOUNT:
				case LST_DSPOPE_EXPENSE:
				case LST_DSPOPE_INCOME:   mask |= FLT_QSEARCH_AMOUNT; break;
			}
		}
	}

	return mask;
}


void list_txn_set_save_column_width(GtkTreeView *treeview, gboolean save_column_width)
{
struct list_txn_data *data;

	data = g_object_get_data(G_OBJECT(treeview), "inst_data");
	if( data )
	{
		data->save_column_width = save_column_width;
	}
}


void list_txn_set_column_acc_visible(GtkTreeView *treeview, gboolean visible)
{
struct list_txn_data *data;
GList *list;
GtkTreeViewColumn *column;

	data = g_object_get_data(G_OBJECT(treeview), "inst_data");

	data->showall = visible;

	list = gtk_tree_view_get_columns( treeview );
	//if acc visible: balance must be invisible
	column = list_txn_get_column(list, LST_DSPOPE_ACCOUNT);
	if(column)
		gtk_tree_view_column_set_visible (column, visible);
	column = list_txn_get_column(list, LST_DSPOPE_BALANCE);
	if(column)
		gtk_tree_view_column_set_visible (column, !visible);


	g_list_free(list);
}


void list_txn_sort_force(GtkTreeSortable *sortable, gpointer user_data)
{
gint sort_column_id;
GtkSortType order;

	DB( g_print("list_txn_sort_force\n") );

	gtk_tree_sortable_get_sort_column_id(sortable, &sort_column_id, &order);
	DB( g_print(" - id %d order %d\n", sort_column_id, order) );

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, order);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), sort_column_id, order);
}


static void list_txn_get_columns(GtkTreeView *treeview)
{
struct list_txn_data *data;
GtkTreeViewColumn *column;
gint i, col_id;

	DB( g_print("\n[list_txn] get columns position/width\n") );
	
	data = g_object_get_data(G_OBJECT(treeview), "inst_data");

	DB( g_print(" nbcol=%d, nbsortid=%d\n", NUM_LST_DSPOPE, gtk_tree_view_get_n_columns (treeview)) );
	
	for(i=0 ; i < NUM_LST_DSPOPE-1 ; i++ )   // -1 'caus: account and blank column
	{
		column = gtk_tree_view_get_column(treeview, i);
		if(column != NULL)
		{
			col_id = gtk_tree_view_column_get_sort_column_id (column);
			if( col_id >= 0 )
			{
			gboolean visible;

				visible = gtk_tree_view_column_get_visible (column);
				if( col_id == LST_DSPOPE_BALANCE)   //keep initial state of balance
					visible = data->tvc_is_visible;

				if( visible )
				{
					PREFS->lst_ope_columns[i] = col_id;
					//5.2 moved here to keep old width in case not visible
					PREFS->lst_ope_col_width[col_id-1] = gtk_tree_view_column_get_width(column);
				}							
				else
					PREFS->lst_ope_columns[i] = -col_id;

				DB( g_print(" col-%2d => %2d '%s' w=%d\n", i, col_id, gtk_tree_view_column_get_title(column), PREFS->lst_ope_col_width[col_id-1] ) );
			}
			else	//should not occurs
				PREFS->lst_ope_columns[i] = 0;
		}
	}
}


static void list_txn_set_columns(GtkTreeView *treeview, gint *col_id)
{
struct list_txn_data *data;
GtkTreeViewColumn *column, *base;
gboolean visible;
GList *list;
gint i = 0;
gint id;

	DB( g_print("\n[list_txn] set columns order/width\n") );
	
	data = g_object_get_data(G_OBJECT(treeview), "inst_data");

#if MYDEBUG == 1
	DB( g_print("\n debug column sortid\n") );

	for(i=0; i < NUM_LST_DSPOPE-1 ; i++ )   // -1 cause account not to be processed
	{
		DB( g_print(" - pos:%2d sortid:%2d\n", i, col_id[i]) );
	}
#endif	


	DB( g_print("\n apply column prefs\n") );

	list = gtk_tree_view_get_columns( treeview );

	base = NULL;

	for(i=0; i < NUM_LST_DSPOPE-1 ; i++ )   // -1 cause account not to be processed
	{
		/* hidden are stored as col_id negative */
		id = ABS(col_id[i]);
		column = list_txn_get_column(list, id);

		//DB( g_print(" - get column %d %p\n", id, column) );
		
		if( column != NULL )
		{
			DB( g_print(" - pos:%2d sortid:%2d (%s)\n", i, col_id[i], gtk_tree_view_column_get_title(column)) );

			gtk_tree_view_move_column_after(treeview, column, base);
			base = column;

			visible = col_id[i] < 0 ? FALSE : TRUE;
			
			/* display exception for detail/import list */
			if(data->list_type != LIST_TXN_TYPE_BOOK)
			{
				if( id == LST_DSPOPE_AMOUNT )
					visible = TRUE;
				
				if( id == LST_DSPOPE_STATUS || id == LST_DSPOPE_EXPENSE || id == LST_DSPOPE_INCOME )
					visible = FALSE;
			}
			
			gtk_tree_view_column_set_visible (column, visible);
			if( id == LST_DSPOPE_BALANCE )
			{
				data->tvc_is_visible = visible;
			}

			if(   id == LST_DSPOPE_INFO
			   || id == LST_DSPOPE_PAYEE
			   || id == LST_DSPOPE_MEMO
			   || id == LST_DSPOPE_CATEGORY
			   || id == LST_DSPOPE_TAGS
			   || id == LST_DSPOPE_ACCOUNT )
			{
				gtk_tree_view_column_set_fixed_width( column, PREFS->lst_ope_col_width[id - 1]);
			}
		}

	}

	g_list_free(list );
}


static void list_txn_sort_column_changed(GtkTreeSortable *sortable, gpointer user_data)
{
struct list_txn_data *data = user_data;
gint id;
GtkSortType order;
gboolean showBalance;
	
	gtk_tree_sortable_get_sort_column_id(sortable, &id, &order);

	DB( g_print("list_txn_columns_changed %d %d\n", id, order) );

	//here save the transaction list columnid and sort order
	PREFS->lst_ope_sort_id    = id;
	PREFS->lst_ope_sort_order = order;

	//manage visibility of balance column
	//showBalance = (id == LST_DSPOPE_DATE && order == GTK_SORT_ASCENDING) ? data->tvc_is_visible : FALSE;
	showBalance = (id == LST_DSPOPE_DATE) ? data->tvc_is_visible : FALSE;
	if(data->showall == TRUE) showBalance = FALSE;
	gtk_tree_view_column_set_visible (data->tvc_balance, showBalance);
}


static void
list_txn_column_popup_menuitem_on_activate (GtkCheckMenuItem *checkmenuitem,
               gpointer          user_data)
{
GtkTreeViewColumn *column = user_data;

	DB( g_print("toggled\n") );

	gtk_tree_view_column_set_visible(column, gtk_check_menu_item_get_active(checkmenuitem) );
}


static gboolean list_txn_column_popup_callback ( GtkWidget *button,
                        GdkEventButton *ev,
                        gpointer user_data )
{
struct list_txn_data *data = user_data;
GtkWidget *menu, *menuitem;
GtkTreeViewColumn *column;
gint i, col_id;

 
	if( ev->button == 3 )
	{
		DB( g_print("should popup\n") );
	
		menu = gtk_menu_new ();
		
		//note: deactive this disable any menuitem action
		g_signal_connect (menu, "selection-done", G_CALLBACK (gtk_widget_destroy), NULL);

		for(i=0 ; i < NUM_LST_DSPOPE-1 ; i++ )   // -1 'caus: account and blank column
		{
			column = gtk_tree_view_get_column(GTK_TREE_VIEW(data->treeview), i);
			if( column != NULL )
			{
				col_id = gtk_tree_view_column_get_sort_column_id (column);

				if( (col_id == -1) 
					|| (col_id == LST_DSPOPE_STATUS) 
					|| (col_id == LST_DSPOPE_ACCOUNT) 
					|| (col_id == LST_DSPOPE_DATE)
					|| (col_id == LST_DSPOPE_BALANCE)
				)
					continue;
				//if( (data->tvc_is_visible == FALSE) && (col_id == LST_DSPOPE_BALANCE) )
				//	continue;
				
				if( (data->list_type == LIST_TXN_TYPE_DETAIL) && 
					(   (col_id == LST_DSPOPE_AMOUNT) 
					|| (col_id == LST_DSPOPE_EXPENSE) 
					|| (col_id == LST_DSPOPE_INCOME)
					) 
				)
					continue;

				menuitem = gtk_check_menu_item_new_with_label ( gtk_tree_view_column_get_title (column) );
				gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), gtk_tree_view_column_get_visible (column) );
				gtk_widget_show (menuitem);
				
				g_signal_connect (menuitem, "activate",
								G_CALLBACK (list_txn_column_popup_menuitem_on_activate), column);
			}
		
		}
	
		gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				      ev->button, ev->time);
	}

    return FALSE;
}


static GtkTreeViewColumn *
list_txn_column_amount_create(gint list_type, gchar *title, gint sortcolumnid, GtkTreeCellDataFunc func)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);

	column = gtk_tree_view_column_new_with_attributes(title, renderer, NULL);
	
	gtk_tree_view_column_set_alignment (column, 0.5);
	//gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	if(list_type == LIST_TXN_TYPE_BOOK)
	{
		gtk_tree_view_column_set_reorderable(column, TRUE);
	}
	gtk_tree_view_column_set_cell_data_func(column, renderer, func, GINT_TO_POINTER(sortcolumnid), NULL);
		
	return column;
}


static GtkTreeViewColumn *
list_txn_column_text_create(gint list_type, gchar *title, gint sortcolumnid, GtkTreeCellDataFunc func, gpointer user_data)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);
	
	column = gtk_tree_view_column_new_with_attributes(title, renderer, NULL);

	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);

	gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	if(list_type == LIST_TXN_TYPE_BOOK)
	{
		gtk_tree_view_column_set_reorderable(column, TRUE);
		gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	}
	gtk_tree_view_column_set_cell_data_func(column, renderer, func, user_data, NULL);

	return column;
}


static GtkTreeViewColumn *
list_txn_column_info_create(gint list_type)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Info"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_info_cell_data_function, GINT_TO_POINTER(1), NULL);

	renderer = gtk_cell_renderer_text_new ();
	/*g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);*/
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_info_cell_data_function, GINT_TO_POINTER(2), NULL);

	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, LST_DSPOPE_INFO);
	if(list_type == LIST_TXN_TYPE_BOOK)
	{
		gtk_tree_view_column_set_reorderable(column, TRUE);
		gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	}

	return column;
}


static void list_txn_destroy( GtkWidget *widget, gpointer user_data )
{
struct list_txn_data *data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print ("\n[list_transaction] destroy event occurred\n") );

	if( data->save_column_width )
	{
		list_txn_get_columns(GTK_TREE_VIEW(data->treeview));
	}
		
	DB( g_print(" - view=%p, inst_data=%p\n", widget, data) );
	g_free(data);
}


Transaction *list_txn_get_active_transaction(GtkTreeView *treeview)
{
GtkTreeModel *model;
GList *list;
Transaction *ope;

	ope = NULL;

	model = gtk_tree_view_get_model(treeview);
	list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(treeview), &model);

	if(list != NULL)
	{
	GtkTreeIter iter;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, LST_DSPOPE_DATAS, &ope, -1);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	return ope;
}


/*
** create our transaction list
** Status, Date, Info, Payee, Memo, (Amount), Expense, Income, Category
*/
GtkWidget *create_list_transaction(gint list_type, gboolean *pref_columns)
{
struct list_txn_data *data;
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column, *col_acc = NULL, *col_status = NULL;


	data = g_malloc0(sizeof(struct list_txn_data));
	if(!data) return NULL;

	data->list_type = list_type;
	data->save_column_width = FALSE;
	
	/* create list store */
	store = gtk_list_store_new(
		1, G_TYPE_POINTER	/*only really used column */   
		);

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	data->treeview = treeview;
	g_object_unref(store);

	//store our window private data
	g_object_set_data(G_OBJECT(treeview), "inst_data", (gpointer)data);
	DB( g_print(" - treeview=%p, inst_data=%p\n", treeview, data) );

	// connect our dispose function
	g_signal_connect (treeview, "destroy", G_CALLBACK (list_txn_destroy), (gpointer)data);
	
	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);
	//gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview),
	//			       COLUMN_DESCRIPTION);

	if(list_type == LIST_TXN_TYPE_BOOK)
		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_MULTIPLE);

	/* column 1: Changes */
	column = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(column, _("Status"));
	col_status = column;

	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_status_cell_data_function, GINT_TO_POINTER(1), NULL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_status_cell_data_function, GINT_TO_POINTER(2), NULL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_status_cell_data_function, GINT_TO_POINTER(3), NULL);

	gtk_tree_view_column_set_sort_column_id (column, LST_DSPOPE_STATUS);
	//gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	//5.2 we always create the column and set it not visible
	column = list_txn_column_text_create(list_type, _("Account"), LST_DSPOPE_ACCOUNT, list_txn_account_cell_data_function, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	col_acc = column;
	// add column popup
	g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	/* column 2: Date */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Date"));
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_date_cell_data_function, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, LST_DSPOPE_DATE);
	//gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );


	column = list_txn_column_info_create(list_type);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );
	
	column = list_txn_column_text_create(list_type, _("Payee"), LST_DSPOPE_PAYEE, list_txn_payee_cell_data_function, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	column = list_txn_column_text_create(list_type, _("Memo"), LST_DSPOPE_MEMO, list_txn_memo_cell_data_function, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	/* column status CLR */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Status"));

	//renderer = gtk_cell_renderer_text_new ();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer, list_txn_clr_cell_data_function, NULL, NULL);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, LST_DSPOPE_CLR);
	//gtk_tree_view_column_set_sort_indicator (column, FALSE);
	//gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );
	
	
	column = list_txn_column_amount_create(list_type, _("Amount"), LST_DSPOPE_AMOUNT, list_txn_amount_cell_data_function);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	column = list_txn_column_amount_create(list_type, _("Expense"), LST_DSPOPE_EXPENSE, list_txn_amount_cell_data_function);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	column = list_txn_column_amount_create(list_type, _("Income"), LST_DSPOPE_INCOME, list_txn_amount_cell_data_function);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	column = list_txn_column_text_create(list_type, _("Category"), LST_DSPOPE_CATEGORY, list_txn_category_cell_data_function, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	column = list_txn_column_text_create(list_type, _("Tags"), LST_DSPOPE_TAGS, list_txn_tags_cell_data_function, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	// add column popup
    g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
		"button-press-event",
		G_CALLBACK ( list_txn_column_popup_callback ),
		data );

	if(list_type == LIST_TXN_TYPE_BOOK)
	{
		column = list_txn_column_amount_create(list_type, _("Balance"), LST_DSPOPE_BALANCE, list_txn_amount_cell_data_function);
		data->tvc_balance = column;
		gtk_tree_view_column_set_clickable(column, FALSE);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
		// add column popup
		g_signal_connect ( G_OBJECT (gtk_tree_view_column_get_button (column)), 
			"button-press-event",
			G_CALLBACK ( list_txn_column_popup_callback ),
			data );
	}
	
  /* column 9: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  /* sort */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_STATUS  , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_STATUS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_DATE    , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_DATE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_INFO    , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_INFO), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_PAYEE   , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_PAYEE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_MEMO    , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_MEMO), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_AMOUNT  , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_AMOUNT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_EXPENSE , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_EXPENSE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_INCOME  , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_INCOME), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_CATEGORY, list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_CATEGORY), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_TAGS    , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_TAGS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_CLR     , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_CLR), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DSPOPE_ACCOUNT , list_txn_sort_iter_compare_func, GINT_TO_POINTER(LST_DSPOPE_ACCOUNT), NULL);

  /* apply user preference for columns */
	list_txn_set_columns(GTK_TREE_VIEW(treeview), pref_columns);

  /* force account column for detail treeview */
	gtk_tree_view_move_column_after(GTK_TREE_VIEW(treeview), col_acc, col_status);
	/* by default book don't display acc column, except shoall */
	if(list_type == LIST_TXN_TYPE_BOOK)
	{
		gtk_tree_view_column_set_visible (col_acc, FALSE);
	}

  /* set initial sort order */
    DB( g_print("set sort to %d %d\n", PREFS->lst_ope_sort_id, PREFS->lst_ope_sort_order) );
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), PREFS->lst_ope_sort_id, PREFS->lst_ope_sort_order);


	/* signals */
	if(list_type == LIST_TXN_TYPE_BOOK)
		g_signal_connect (GTK_TREE_SORTABLE(store), "sort-column-changed", G_CALLBACK (list_txn_sort_column_changed), data);

	return(treeview);
}


