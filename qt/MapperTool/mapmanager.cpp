#include "mapmanager.h"

MapManager::MapManager()
{
    glGenTextures( 1, &m_texture );
}

MapManager::~MapManager()
{
    glDeleteTextures( 1, &m_texture );
}


void MapManager::setMat( cv::Mat newMat )
{
    m_map = newMat.clone();

    addHistoryState();
    updateTexture();
}

cv::Mat MapManager::getMat()
{
    return m_map;
}

QSize MapManager::getSize()
{
    return QSize(m_map.cols, m_map.rows);
}

GLuint MapManager::getTexture()
{
    return m_texture;
}

void MapManager::createFromTexture( GLuint texture )
{
    GLint width, height, glFormat;
    int depth=0, channels=0, type;

    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &glFormat);

    switch(glFormat) {
    case GL_RGB8:
        depth = CV_8U;
        channels = 3;
        break;
    case GL_RGBA8:
        depth = CV_8U;
        channels = 4;
        break;
    case GL_RGB16:
        depth = CV_16U;
        channels = 3;
        break;
    case GL_RGBA16:
        depth = CV_16U;
        channels = 4;
        break;
    default:
        qDebug() << "Unsupported texture format";
        return;
        break;
    }

    type = (depth == CV_8U)?
                ((channels == 3)?CV_8UC3:CV_8UC4):
                ((channels == 3)?CV_16UC3:CV_16UC4);
    cv::Mat newMat(height, width, type);

    glGetTexImage(GL_TEXTURE_2D,0,
                 (channels == 3)?GL_BGR_EXT:GL_BGRA_EXT,
                 (depth == CV_8U)?GL_UNSIGNED_BYTE:GL_UNSIGNED_SHORT,
                 newMat.data);

    m_fileName.clear();
    resetHistory();

    setMat(newMat);
}

bool MapManager::load( QString fileName )
{
    if( fileName.isNull() ) {
        fileName = m_fileName;
        if( fileName.isNull() ) {
            return false;
        }
    }

    cv::Mat loadedMat = cv::imread( fileName.toStdString(), CV_LOAD_IMAGE_UNCHANGED );
    if( !loadedMat.data ) {
        qDebug() << "Could not load map from '" << fileName << "'.";
        return false;
    }

    resetHistory();
    m_map = loadedMat;

    addHistoryState();
    updateTexture();

    m_fileName = fileName;
    return true;
}

bool MapManager::save( QString fileName )
{
    if( fileName.isNull() ) {
        fileName = m_fileName;
        if( fileName.isNull() ) {
            return false;
        }
    }

    if( !cv::imwrite( fileName.toStdString(), m_map )) {
        qDebug() << "Could not save map to '" << fileName << "'.";
        return false;
    }

    m_fileName = fileName;
    return true;
}

bool MapManager::undo()
{
    if( m_historyIndex == 0 ) {
        return false;
    }

    m_historyIndex--;
    m_map = m_history.at(m_historyIndex);

    updateTexture();

    if( m_historyIndex == 0 ) {
        // no more undos available
        return false;
    }

    return true;
}

bool MapManager::redo()
{
    if( m_historyIndex == m_history.count()-1 ) {
        return false;
    }

    m_historyIndex++;
    m_map = m_history.at(m_historyIndex);

    updateTexture();

    if( m_historyIndex == m_history.count()-1 ) {
        // no more redos available
        return false;
    }

    return true;
}

void MapManager::addHistoryState()
{
    if( m_historyIndex < (m_history.count()-1) )
        resetHistory( m_historyIndex );

    m_history.push_back( m_map );
    m_historyIndex = m_history.count()-1;
}

void MapManager::resetHistory( int fromIndex )
{
    QVectorIterator<cv::Mat> historyItr(m_history);
    historyItr.toBack();
    while( historyItr.hasPrevious() && m_history.count() > fromIndex ) {
        cv::Mat mat = historyItr.previous();
        mat.release();
        m_history.pop_back();
    }
}

void MapManager::updateTexture()
{
    GLenum type, format;

    switch (m_map.depth()) {
    case CV_8U:
        format = GL_UNSIGNED_BYTE;
        break;
    case CV_16U:
        format = GL_UNSIGNED_SHORT;
        break;
    }

    switch (m_map.channels()) {
    case 3:
        type = GL_BGR_EXT;
        break;
    case 4:
        type = GL_BGRA_EXT;
        break;
    }

    GLint glFormat = (type == GL_BGR_EXT)?
                ((format == GL_UNSIGNED_BYTE)?GL_RGB8:GL_RGB16):
                ((format == GL_UNSIGNED_BYTE)?GL_RGBA8:GL_RGBA16);

    glBindTexture( GL_TEXTURE_2D, m_texture );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    glTexImage2D( GL_TEXTURE_2D, 0, glFormat, m_map.cols, m_map.rows, 0, type, format, m_map.data);
}
