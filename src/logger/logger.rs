use flexi_logger::{FileSpec, Logger, WriteMode};
use std::error::Error;

pub fn init_logger(log_level: &str) -> Result<(), Box<dyn Error>> {
    Logger::try_with_str(log_level)?
        .log_to_file(
            FileSpec::default()
                .directory("logs")
                .basename("wasol")
                .suppress_timestamp()
                .suffix("log")
        )
        .write_mode(WriteMode::BufferAndFlush)
        .start()?;
    Ok(())
}