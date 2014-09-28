/*****************************************************************************
 * raw.c: pes muxer
 *****************************************************************************
 *
 * Author: Lorenzo Pallara <l.pallara@avalpa.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "output.h"

#define PES_HEADER_SIZE 19
#define SYSTEM_CLOCK 90000
#define PTS_MAX 8589934592LL

typedef struct
{
    FILE *file;
    u_int8_t *header_written;
    int header_written_size;
    u_int8_t pes_header[PES_HEADER_SIZE];
} pes_hnd_t;

void stamp_pes_time( u_int64_t pes_time, u_int8_t* buffer ) 
{
    if ( buffer )
    {
        buffer[0] = ( ( pes_time >> 29 ) & 0x0F ) | 0x01;
        buffer[1] = ( ( pes_time >> 22 ) & 0xFF );
        buffer[2] = ( ( pes_time >> 14 ) & 0xFF ) | 0x01;
        buffer[3] = ( ( pes_time >>  7 ) & 0xFF );
        buffer[4] = ( ( pes_time <<  1 ) & 0xFF ) | 0x01;
    }
}

static int open_file( char *filename, hnd_t *handle, cli_output_opt_t *opt )
{
    pes_hnd_t *pes_handle = 0;

    *handle = malloc( sizeof( pes_hnd_t ) );
    if ( !handle )
        return -1;
    memset( *handle, 0, sizeof( pes_hnd_t ) );

    pes_handle = (pes_hnd_t *)*handle;
    if ( !strcmp( filename, "-" ) )
        pes_handle->file = stdout;
    else
        pes_handle->file = fopen( filename, "w+b" );
    if ( !pes_handle->file )
    {
        free( pes_handle );
        return -1;
    }
    pes_handle->pes_header[2] = 0x01;
    pes_handle->pes_header[3] = 0xE0; /* an usual stream id */
    pes_handle->pes_header[6] = 0x80; /* no scrambling, no priority, no alignment defined, no copyright, no copy */

    return 0;
}

static int set_param( hnd_t handle, x264_param_t *param )
{
    return 0;
}

static int write_headers( hnd_t handle, x264_nal_t *nal )
{

    pes_hnd_t *pes_handle = (pes_hnd_t *)handle;
    if ( !pes_handle )
        return -1;

    /* memcpy the headers so to write them after the pes header but before the frame */
    pes_handle->header_written_size = nal[0].i_payload + nal[1].i_payload + nal[2].i_payload;
    if ( pes_handle->header_written_size < 0 )
    {
        pes_handle->header_written_size = 0;
        return -1;
    } 
    else if ( pes_handle->header_written_size == 0 )
        return 0;

    /* prevents memory leak if write headers called twice, it should not happen anyway */
    if ( pes_handle->header_written )
        free( pes_handle->header_written );

    pes_handle->header_written = malloc( pes_handle->header_written_size );
    if ( pes_handle->header_written == 0 )
    {
        pes_handle->header_written_size = 0;
        return -1;
    }

    memcpy( pes_handle->header_written, nal[0].p_payload, pes_handle->header_written_size );
    memcpy( pes_handle->header_written, "pluto", 6 );

    return pes_handle->header_written_size;
}

static int write_frame( hnd_t handle, uint8_t *nalu, int size, x264_picture_t *picture )
{
    pes_hnd_t *pes_handle = (pes_hnd_t *)handle;
    int result = -1;
    int current_pes_header_size = PES_HEADER_SIZE;

    if ( picture->i_pts != picture->i_dts )
    {
        pes_handle->pes_header[7] = 0xC0; /* pts and dts, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
        pes_handle->pes_header[8] = 0x0A; /* size*/ 
        stamp_pes_time( (uint64_t) ( ( picture->hrd_timing.dpb_output_time * SYSTEM_CLOCK ) + 0.5 ) % PTS_MAX, pes_handle->pes_header + 9);
        pes_handle->pes_header[9] &= 0x0F;
        pes_handle->pes_header[9] |= 0x30;
        stamp_pes_time( (uint64_t) ( ( picture->hrd_timing.cpb_removal_time * SYSTEM_CLOCK ) + 0.5 ) % PTS_MAX, pes_handle->pes_header + 14);
        pes_handle->pes_header[14] &= 0x0F;
        pes_handle->pes_header[14] |= 0x10;
    }
    else
    {
        current_pes_header_size = PES_HEADER_SIZE - 5;
        pes_handle->pes_header[7] = 0x80; /* pts only, no escr, no es rate, no dsm trick, no extension flag, no additional copy info, no crc flag */
        pes_handle->pes_header[8] = 0x05; /* size*/ 
        stamp_pes_time( (uint64_t) ( ( picture->hrd_timing.dpb_output_time * SYSTEM_CLOCK ) + 0.5 ) % PTS_MAX, pes_handle->pes_header + 9);
        pes_handle->pes_header[9] &= 0x0F;
        pes_handle->pes_header[9] |= 0x30;
    }

    if ( !fwrite( pes_handle->pes_header, current_pes_header_size, 1, pes_handle->file ) )
        return -1;

    if ( pes_handle->header_written )
    {
        result = fwrite( pes_handle->header_written, pes_handle->header_written_size, 1, pes_handle->file );
        free( pes_handle->header_written );
        pes_handle->header_written = 0;
        pes_handle->header_written_size = 0;
        if ( !result )
            return -1;
    }

    if ( fwrite( nalu, size, 1, pes_handle->file ) )
        return size;
    return -1;
}

static int close_file( hnd_t handle, int64_t largest_pts, int64_t second_largest_pts )
{
    int result = -1;
    pes_hnd_t *pes_handle = (pes_hnd_t *)handle;

    if ( pes_handle )
    {
        if ( pes_handle->file )
            result = fclose( pes_handle->file );
        if ( pes_handle->header_written )
        {
            free( pes_handle->header_written );
            pes_handle->header_written = 0;
        }
        free( pes_handle );
    }

    return result;
}

const cli_output_t raw_output = { open_file, set_param, write_headers, write_frame, close_file };
