#include "gtest/gtest.h"

#include "IMediaLibrary.h"
#include "ILabel.h"

class MLTest : public testing::Test
{
    public:
        static IMediaLibrary* ml;

    protected:
        virtual void SetUp()
        {
            ml = MediaLibraryFactory::create();
            bool res = ml->initialize( "test.db" );
            ASSERT_TRUE( res );
        }

        virtual void TearDown()
        {
            delete ml;
            unlink("test.db");
        }
};

IMediaLibrary* MLTest::ml;

TEST_F( MLTest, Init )
{
    // only test for correct test fixture behavior
}

TEST_F( MLTest, InsertFile )
{
    IFile* f = ml->addFile( "/dev/null" );
    ASSERT_TRUE( f != NULL );

    ASSERT_EQ( f->playCount(), 0 );
    ASSERT_TRUE( f->albumTrack() == NULL );
    ASSERT_TRUE( f->showEpisode() == NULL );

    std::vector<IFile*> files = ml->files();
    ASSERT_EQ( files.size(), 1u );
    ASSERT_EQ( files[0]->mrl(), f->mrl() );

    delete f;
}

TEST_F( MLTest, AddLabel )
{
    IFile* f = ml->addFile( "/dev/null" );
    ILabel* l1 = f->addLabel( "sea otter" );
    ILabel* l2 = f->addLabel( "cony the cone" );

    ASSERT_TRUE( l1 != NULL );
    ASSERT_TRUE( l2 != NULL );

    std::vector<ILabel*> labels = f->labels();
    ASSERT_EQ( labels.size(), 2u );
    ASSERT_EQ( labels[0]->name(), "sea otter" );
    ASSERT_EQ( labels[1]->name(), "cony the cone" );

    delete l1;
    delete l2;
    delete f;
}
