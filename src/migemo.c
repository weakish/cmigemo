/* vim:set ts=8 sts=4 sw=4 tw=0 */
/*
 * migemo.c -
 *
 * Written By:  Muraoka Taro <koron@tka.att.ne.jp>
 * Last Change: 07-Aug-2001.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wordbuf.h"
#include "wordlist.h"
#include "mnode.h"
#include "rxgen.h"
#include "romaji.h"
#include "filename.h"
#include "migemo.h"

/* migemo�I�u�W�F�N�g */
struct _migemo
{
    mnode* node;
    romaji* roma2hira;
    romaji* hira2kata;
};

/*
 * migemo interfaces
 */

    migemo*
migemo_open(char* dict)
{
    migemo *obj = NULL;
    FILE *fp = NULL;

    if (dict
	    && (fp = fopen(dict, "rt")) /* �����t�@�C���̑��݊m�F */
	    && (obj = (migemo*)malloc(sizeof(migemo))))
    {
	char dir[_MAX_PATH], roma_dict[_MAX_PATH], kata_dict[_MAX_PATH];

	obj->node = mnode_open(fp);
	obj->roma2hira = romaji_open();
	obj->hira2kata = romaji_open();
	if (!obj->node || !obj->roma2hira || !obj->hira2kata)
	{
	    migemo_close(obj);
	    obj = NULL;
	}

	filename_directory(dir, dict);
	/*printf("dir=\"%s\"\n", dir);*/
	strcpy(roma_dict, strlen(dir) ? dir : ".");
	strcpy(kata_dict, strlen(dir) ? dir : ".");
	strcat(roma_dict, "/romaji.dat");
	strcat(kata_dict, "/hira2kata.dat");
	romaji_load(obj->roma2hira, roma_dict);
	romaji_load(obj->hira2kata, kata_dict);
    }

    if (fp)
	fclose(fp); /*  �����t�@�C���N���[�Y */
    return obj;
}

    void
migemo_close(migemo* obj)
{
    if (obj)
    {
	if (obj->hira2kata)
	    romaji_close(obj->hira2kata);
	if (obj->roma2hira)
	    romaji_close(obj->roma2hira);
	if (obj->node)
	    mnode_close(obj->node);
	free(obj);
    }
}

#if 0
    void
migemo_query_proc(mnode* p, void* data)
{
    wordbuf *buf = (wordbuf*)data;
    wordlist *list = p->list;

    for (; list; list = list->next)
    {
	if (wordbuf_last(buf) > 0)
	    wordbuf_add(buf, ' ');
	wordbuf_cat(buf, list->ptr);
    }
}

    unsigned char*
migemo_query(migemo* p, unsigned char* query)
{
    unsigned char* answer = NULL;
    mnode *pnode;

    /*printf("migemo_query(p=%p, query=\"%s\")\n", p, query);*/
    if (pnode = mnode_query(p->node, query))
    {
	wordbuf *pwordbuf;

	pwordbuf = wordbuf_open();
	/* �o�b�t�@��p�ӂ��čċA�Ńo�b�t�@�ɏ������܂��� */
	mnode_traverse(pnode, migemo_query_proc, pwordbuf);
	answer = strdup(wordbuf_get(pwordbuf));
	wordbuf_close(pwordbuf);
    }
    /*printf("  pnode=%p, answer=%p\n", query, pnode, answer);*/

    return answer;
}

    void
migemo_release(migemo* p, unsigned char* string)
{
    free(string);
}
#else
/*
 * query version 2
 */

    static void
migemo_query_proc(mnode* p, void* data)
{
    rxgen *rx = (rxgen*)data;
    wordlist *list = p->list;

    for (; list; list = list->next)
	rxgen_add(rx, list->ptr);
}

/*
 * ���[�}���ϊ����s���S���������ɁA[aiueon]�����ĕϊ����Ă݂�B
 */
    static void
migemo_query_stub(migemo* object, rxgen* rx, unsigned char* query)
{
    int len;
    char *buf;
    unsigned char candidate[] = "aiueon", *ptr;

    if (!(len = strlen(query)))
	return;
    if (!(buf = malloc(len + 2))) /* NUL�Ɗg�������p */
	return;
    strcpy(buf, query);
    buf[len + 1] = '\0';

    for (ptr = candidate; *ptr; ++ptr)
    {
	unsigned char *stop, *hira, *kata;

	buf[len] = *ptr;
	hira = romaji_convert(object->roma2hira, buf, &stop);

	if (!stop)
	{
	    rxgen_add(rx, hira);
	    /* �Љ���������𐶐������ɉ����� */
	    kata = romaji_convert(object->hira2kata, hira, NULL);
	    rxgen_add(rx, kata);
	    romaji_release(object->hira2kata, kata);
	}
	romaji_release(object->roma2hira, hira);
    }
}

    unsigned char*
migemo_query(migemo* object, unsigned char* query)
{
    unsigned char* answer = NULL;
    mnode *pnode;
    rxgen *rx = rxgen_open();

    if (rx = rxgen_open())
    {
	unsigned char *stop, *hira, *kata;

	rxgen_add(rx, query);

	/* ������������𐶐������ɉ����� */
	hira = romaji_convert(object->roma2hira, query, &stop);
	if (!stop)
	{
	    rxgen_add(rx, hira);
	    /* �Љ���������𐶐������ɉ����� */
	    kata = romaji_convert(object->hira2kata, hira, NULL);
	    rxgen_add(rx, kata);
	    romaji_release(object->hira2kata, kata);
	}
	else
	    migemo_query_stub(object, rx, query);
	romaji_release(object->roma2hira, hira);

	/* �o�b�t�@��p�ӂ��čċA�Ńo�b�t�@�ɏ������܂��� */
	if (pnode = mnode_query(object->node, query))
	    mnode_traverse(pnode, migemo_query_proc, rx);

	answer = rxgen_generate(rx);
	rxgen_close(rx);
    }

    return answer;
}

    void
migemo_release(migemo* p, unsigned char* string)
{
    rxgen_release(NULL, string);
}
#endif