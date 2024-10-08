/*
 * disk/uss_john_young_protection.c
 *
 * Custom format as used U.S.S. John Young by Rainbow Arts
 *
 * Written in 2024 by Keith Krellwitz
 *
 * RAW TRACK LAYOUT:
 *  u16 0x6cb1 Sync
 *  u32 dat[ti->len/4]
 *
 * There is no checksum and it appears the data is not used
 * even though it is decoded.  Created my own checksum, but only
 * had a single dump to test. 
 * 
 * TRKTYP_uss_john_young_protection data layout:
 *  u8 sector_data[6144]
 * 
 */

#include <libdisk/util.h>
#include <private/disk.h>

static void *uss_john_young_protection_write_raw(
    struct disk *d, unsigned int tracknr, struct stream *s)
{
    struct track_info *ti = &d->di->track[tracknr];

    while (stream_next_bit(s) != -1) {

        uint32_t raw[2], dat[ti->len/4], sum;
        unsigned int i;
        char *block;

        if ((uint16_t)s->word != 0x6cb1)
            continue;
        ti->data_bitoff = s->index_offset_bc - 15;


        for (i = sum = 0; i < ti->len/4; i++) {
            if (stream_next_bytes(s, raw, 8) == -1)
                goto fail;
            mfm_decode_bytes(bc_mfm_even_odd, 4, raw, &dat[i]);
            sum += be32toh(dat[i]);
        }

        if (sum != 0x28f53cb9)
            continue;

        stream_next_index(s);
        block = memalloc(ti->len);
        memcpy(block, dat, ti->len);
        set_all_sectors_valid(ti);
        ti->total_bits = s->track_len_bc;
        return block;
    }

fail:
    return NULL;
}

static void uss_john_young_protection_read_raw(
    struct disk *d, unsigned int tracknr, struct tbuf *tbuf)
{
    struct track_info *ti = &d->di->track[tracknr];
    uint32_t *dat = (uint32_t *)ti->dat;
    unsigned int i;

    tbuf_bits(tbuf, SPEED_AVG, bc_raw, 16, 0x6cb1);

    for (i = 0; i < ti->len/4; i++) {
        tbuf_bits(tbuf, SPEED_AVG, bc_mfm_even_odd, 32, be32toh(dat[i]));
    }
}

struct track_handler uss_john_young_protection_handler = {
    .bytes_per_sector = 6144,
    .nr_sectors = 1,
    .write_raw = uss_john_young_protection_write_raw,
    .read_raw = uss_john_young_protection_read_raw
};


/*
 * Local variables:
 * mode: C
 * c-file-style: "Linux"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
