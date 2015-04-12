#ifndef IALBUM_H
#define IALBUM_H

#include "IMediaLibrary.h"

class IAlbumTrack;

class IAlbum
{
    public:
        virtual ~IAlbum() {}
        virtual unsigned int id() const = 0;
        virtual const std::string& title() const = 0;
        virtual time_t releaseDate() const = 0;
        virtual bool setReleaseDate( time_t date ) = 0;
        virtual const std::string& shortSummary() const = 0;
        virtual bool setShortSummary( const std::string& summary ) = 0;
        virtual const std::string& artworkUrl() const = 0;
        virtual bool setArtworkUrl( const std::string& artworkUrl ) = 0;
        virtual std::vector<std::shared_ptr<IAlbumTrack>> tracks() const = 0;
        virtual AlbumTrackPtr addTrack( const std::string& title, unsigned int trackId ) = 0;
        virtual bool destroy() = 0;
};

#endif // IALBUM_H
